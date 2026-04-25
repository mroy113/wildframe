#pragma once

/// \file
/// `PipelineStage` — abstract base for a single step in the per-image
/// pipeline (TB-02, M6-01, NFR-3, handoff §10, §12).
///
/// Concrete stages live in their owning modules (`RawStage` in
/// `wildframe_raw`, `DetectStage` in `wildframe_detect`, …) and are
/// handed to the orchestrator as a
/// `std::vector<std::unique_ptr<PipelineStage>>`. The orchestrator
/// invokes them in registration order for every job. Each stage reads
/// upstream fields from the mutable `StageContext` it receives and
/// writes its own output back into the same struct for downstream
/// stages to consume.
///
/// Exception policy (`docs/STYLE.md` §3): a stage's implementation may
/// throw its module's Wildframe error type on per-image failure. The
/// orchestrator currently lets the exception propagate out of `Run`;
/// M6-04 adds per-image error isolation and manifest rows.
///
/// `StageContext` starts minimal (just the `ImageJob` under analysis)
/// and grows incrementally as TB-03..TB-07 land the real types their
/// stages produce — each task adds one `std::optional<...>` field
/// alongside the type that backs it. The struct-of-optional shape was
/// TB-02's design call per the backlog; a variant would force every
/// consumer to switch on a runtime tag for state that is statically
/// known from the stage ordering.

#include <optional>
#include <string_view>

#include "wildframe/detect/detect.hpp"
#include "wildframe/focus/focus.hpp"
#include "wildframe/ingest/image_job.hpp"
#include "wildframe/metadata/deterministic_metadata.hpp"
#include "wildframe/raw/preview_image.hpp"

namespace wildframe::orchestrator {

/// Per-image state threaded through the stage chain. Stages read
/// upstream fields and write their own output; the orchestrator
/// constructs one instance per job and discards it when the pipeline
/// for that job finishes or throws.
struct StageContext {
  /// The job under analysis. Stored by value — `ImageJob` copy cost
  /// is one `std::filesystem::path` plus a few scalars, paid once
  /// per job, and holding by value avoids a reference data member
  /// (which would delete copy-assignment and trip
  /// `cppcoreguidelines-avoid-const-or-ref-data-members`).
  ingest::ImageJob job;

  /// Decoded preview from `wildframe_raw::ExtractPreview`, populated
  /// by `RawStage` (TB-03) and consumed by downstream stages
  /// (TB-04 / M3-*, TB-05 / M4-*). `std::nullopt` until `RawStage`
  /// runs — dereferencing before that is a programming error.
  std::optional<raw::PreviewImage> preview;

  /// Deterministic camera/shot metadata from
  /// `wildframe_metadata::ReadExif`, populated by
  /// `MetadataReadStage` (TB-06) and consumed by
  /// `MetadataWriteStage` (TB-07 / M5-08) as the source of the
  /// standard-namespace EXIF fields it mirrors onto the sidecar.
  /// `std::nullopt` until `MetadataReadStage` runs.
  std::optional<metadata::DeterministicMetadata> exif;

  /// Detector output from `wildframe_detect::Detect`, populated by
  /// `DetectStage` (TB-04) and consumed by `FocusStage` (TB-05) and
  /// `MetadataWriteStage` (TB-07 / M5-08). `std::nullopt` until
  /// `DetectStage` runs.
  std::optional<detect::DetectionResult> detection;

  /// Focus / keeper scores from `wildframe_focus::Score`, populated
  /// by `FocusStage` (TB-05) and consumed by `MetadataWriteStage`
  /// (TB-07 / M5-08). `std::nullopt` until `FocusStage` runs.
  std::optional<focus::FocusResult> focus;
};

/// Per-stage return value. Empty in the Sprint 2 skeleton: stages
/// signal per-image failure via caught exceptions (`docs/STYLE.md` §3)
/// and successful outputs flow through `StageContext`, not the return
/// channel. Reserved as a named type so per-stage metadata (diagnostic
/// flags, skip reasons) can land without re-shaping the virtual
/// signature when TB-08 / M6-05 need it.
struct StageResult {};

/// Abstract base. Non-copyable and non-movable because the orchestrator
/// holds stages by `std::unique_ptr<PipelineStage>` and never slices
/// or relocates them. Concrete subclasses may re-enable move/copy if
/// genuinely needed, but the MVP five stages do not.
class PipelineStage {
 public:
  PipelineStage() = default;
  PipelineStage(const PipelineStage&) = delete;
  PipelineStage& operator=(const PipelineStage&) = delete;
  PipelineStage(PipelineStage&&) = delete;
  PipelineStage& operator=(PipelineStage&&) = delete;
  virtual ~PipelineStage() = default;

  /// Short, stable name used by the orchestrator for per-stage log
  /// lines and, once TB-08 lands, as the manifest `stage_timings_ms`
  /// key. Distinct from `docs/STYLE.md` §4.5 module tags because
  /// stages and modules are not 1:1 (`wildframe_metadata` owns both
  /// a read and a write stage). The returned view must outlive every
  /// call — concrete stages back it with a `constexpr` literal or a
  /// member `std::string`.
  [[nodiscard]] virtual std::string_view Name() const noexcept = 0;

  /// Run the stage for one job. Reads upstream fields from
  /// `context`, writes the stage's output back into `context`, and
  /// returns a `StageResult` (currently empty). Throws the owning
  /// module's Wildframe error type on expected per-image failure.
  virtual StageResult Process(StageContext& context) = 0;
};

}  // namespace wildframe::orchestrator
