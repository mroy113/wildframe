#pragma once

/// \file
/// `MakeMetadataWriteStage` — factory for the orchestrator's per-image
/// XMP sidecar write step (TB-07, handoff §12, `docs/METADATA.md` §4).
///
/// Follows the stage-implementation pattern from
/// `docs/ARCHITECTURE.md` §4.5: the concrete `MetadataWriteStage` lives
/// in the anonymous namespace of `metadata_write_stage.cpp` so it can
/// inherit from `orchestrator::PipelineStage` without pulling the full
/// base header into `wildframe_metadata`'s public interface. Forward-
/// declaring the base keeps consumers (`src/run.cpp`, tests) from
/// transitively picking up orchestrator's include surface through this
/// header.
///
/// The stage captures `analysis_timestamp` at construction time so
/// every sidecar written over the stage's lifetime reports the same
/// instant, per `docs/METADATA.md` §7.2 ("Timestamps are written once
/// per run — a single batch that processes 500 images records the same
/// `analysis_timestamp` on every sidecar it writes in that run").
/// Taking the clock as a parameter rather than reading it inside
/// `Process` keeps `docs/STYLE.md` §2.12's single-clock-boundary rule.

#include <chrono>
#include <memory>

namespace wildframe::orchestrator {
class PipelineStage;
}  // namespace wildframe::orchestrator

namespace wildframe::metadata {

/// Construct a stage that calls `WriteProvenanceSidecar` on the current
/// `StageContext::job.path`, writing the five Sprint 2 provenance
/// fields to `<path>.xmp`. The sidecar is created if absent and
/// updated in place otherwise; any non-`wildframe_provenance:*` XMP
/// nodes already on the sidecar are preserved (`docs/METADATA.md`
/// §1.3). Stage ownership is move-only and passes to the orchestrator's
/// stage vector.
[[nodiscard]] std::unique_ptr<orchestrator::PipelineStage>
MakeMetadataWriteStage(
    std::chrono::system_clock::time_point analysis_timestamp);

}  // namespace wildframe::metadata
