#pragma once

/// \file
/// `Orchestrator` — sequential per-image pipeline runner (TB-02, §12,
/// NFR-3, NFR-9).
///
/// MVP shape: a plain `for`-loop over jobs, on the calling thread,
/// over a registration-order-preserving `std::vector<std::unique_ptr<
/// PipelineStage>>`. M6-02 / M6-03 later replace the loop with a job
/// queue + background worker thread without changing the public
/// constructor or `Run` signature; M6-04 adds per-image error
/// isolation. Until M6-04, any stage exception propagates out of
/// `Run` and aborts the batch.
///
/// Thread-safety (NFR-9 contract): an `Orchestrator` is
/// thread-compatible — a single instance may be driven by at most one
/// thread at a time. The MVP CLI drives it synchronously on the
/// calling thread; the GUI will drive it from a dedicated worker
/// (ARCHITECTURE §5). Multiple concurrent `Orchestrator` instances
/// against disjoint job sets are safe.

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include "wildframe/ingest/image_job.hpp"
#include "wildframe/orchestrator/pipeline_stage.hpp"

namespace wildframe::orchestrator {

/// One progress tick. Emitted by `Orchestrator::Run` after every job
/// completes successfully (failures currently abort the run; M6-04
/// will emit a final tick per failure too). The GUI (M6-06, M7-02)
/// marshals these onto the Qt main thread for progress-bar updates.
struct ProgressUpdate {
  /// Number of jobs that have finished their stage chain so far,
  /// including the one this tick announces.
  std::size_t completed = 0;

  /// Total jobs the run was started with. Constant across a run.
  std::size_t total = 0;

  /// Absolute path of the job that just completed. Copied in so the
  /// callback survives past the lifetime of the source `ImageJob`.
  std::filesystem::path current_job;
};

/// Whether every job completed cleanly. `kPartial` lands with M6-04's
/// per-image error isolation; today a stage exception aborts `Run` so
/// `kCompleted` is the only observed value on a successful return.
/// `cancelled` status (FR-9) arrives with M6-08. Explicit 8-bit
/// underlying type per `performance-enum-size`.
enum class RunStatus : std::uint8_t {
  kCompleted,
  kPartial,
};

/// Batch summary. Returned by `Orchestrator::Run` for the CLI entry
/// point and, later, the GUI. Structure tracks TB-08's manifest
/// shape loosely but does not replace the manifest itself — M6-05
/// extends this struct as richer status becomes observable.
struct RunResult {
  RunStatus status = RunStatus::kCompleted;
  std::size_t jobs_total = 0;
  std::size_t jobs_completed = 0;
};

/// Optional per-job progress callback. Move-friendly and copyable
/// (plain `std::function`) so the CLI/GUI can hand in a lambda or a
/// bound member function.
using ProgressCallback = std::function<void(ProgressUpdate)>;

class Orchestrator {
 public:
  /// Construct with a (possibly empty) registration-order list of
  /// stages, the resolved manifest directory, and an optional
  /// progress callback. Stage ownership is transferred in — the
  /// caller moves a `std::vector<std::unique_ptr<PipelineStage>>`
  /// into the orchestrator and loses direct access to the stages.
  ///
  /// `manifest_dir` is stored as-is; the orchestrator does not create
  /// the directory or open a manifest file in the Sprint 2 skeleton
  /// (TB-08 adds the manifest writer).
  Orchestrator(std::vector<std::unique_ptr<PipelineStage>> stages,
               std::filesystem::path manifest_dir,
               ProgressCallback on_progress = {});

  Orchestrator(const Orchestrator&) = delete;
  Orchestrator& operator=(const Orchestrator&) = delete;
  Orchestrator(Orchestrator&&) = delete;
  Orchestrator& operator=(Orchestrator&&) = delete;
  ~Orchestrator() = default;

  /// Run every stage in registration order for every job in `jobs`.
  /// Sequential over jobs and stages on the calling thread. Stage
  /// exceptions propagate — M6-04 catches them and writes manifest
  /// rows. Progress callback, if set, fires once per job after its
  /// stage chain completes.
  RunResult Run(std::span<const ingest::ImageJob> jobs);

  /// The resolved manifest directory passed into the constructor.
  /// Exposed for diagnostics and for TB-08's manifest writer, which
  /// will thread it through at construction time once it lands.
  [[nodiscard]] const std::filesystem::path& manifest_dir() const noexcept {
    return manifest_dir_;
  }

 private:
  std::vector<std::unique_ptr<PipelineStage>> stages_;
  std::filesystem::path manifest_dir_;
  ProgressCallback on_progress_;
};

}  // namespace wildframe::orchestrator
