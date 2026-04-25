#pragma once

/// \file
/// `MakeMetadataReadStage` — factory for the orchestrator's per-image
/// deterministic-metadata read step (TB-06, handoff §12).
///
/// Follows the stage-implementation pattern from
/// `docs/ARCHITECTURE.md` §4.5: the concrete `MetadataReadStage` lives
/// in the anonymous namespace of `metadata_read_stage.cpp` so it can
/// inherit from `orchestrator::PipelineStage` without pulling the full
/// base header into `wildframe_metadata`'s public interface. Forward-
/// declaring the base keeps consumers (`src/run.cpp`, tests) from
/// transitively picking up orchestrator's include surface through this
/// header.

#include <memory>

namespace wildframe::orchestrator {
class PipelineStage;
}  // namespace wildframe::orchestrator

namespace wildframe::metadata {

/// Construct a stage that calls `ReadExif` on the current
/// `StageContext::job.path` and writes the result into
/// `StageContext::exif` for downstream stages (TB-07 / M5-08, which
/// consume the deterministic fields at XMP-write time). The returned
/// pointer is move-only and owned by the caller (typically the
/// orchestrator's stage vector).
[[nodiscard]] std::unique_ptr<orchestrator::PipelineStage>
MakeMetadataReadStage();

}  // namespace wildframe::metadata
