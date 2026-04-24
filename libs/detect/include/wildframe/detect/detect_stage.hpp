#pragma once

/// \file
/// `MakeDetectStage` — factory for the orchestrator's per-image
/// detection step (TB-04, handoff §12).
///
/// Follows the stage-implementation pattern from
/// `docs/ARCHITECTURE.md` §4.5: the concrete `DetectStage` lives in
/// the anonymous namespace of `detect_stage.cpp` so it can inherit
/// from `orchestrator::PipelineStage` without pulling the full base
/// header into `wildframe_detect`'s public interface. Forward-
/// declaring the base keeps consumers (`src/run.cpp`, tests) from
/// transitively picking up orchestrator's include surface through
/// this header.

#include <memory>

namespace wildframe::orchestrator {
class PipelineStage;
}  // namespace wildframe::orchestrator

namespace wildframe::detect {

/// Construct a stage that calls `Detect` on the `StageContext::preview`
/// populated by `RawStage` and writes the result into
/// `StageContext::detection` for downstream stages. The returned
/// pointer is move-only and owned by the caller (typically the
/// orchestrator's stage vector).
[[nodiscard]] std::unique_ptr<orchestrator::PipelineStage> MakeDetectStage();

}  // namespace wildframe::detect
