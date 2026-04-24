#pragma once

/// \file
/// `MakeFocusStage` — factory for the orchestrator's per-image focus
/// scoring step (TB-05, handoff §12).
///
/// Follows the stage-implementation pattern from
/// `docs/ARCHITECTURE.md` §4.5: the concrete `FocusStage` lives in
/// the anonymous namespace of `focus_stage.cpp` so it can inherit
/// from `orchestrator::PipelineStage` without pulling the full base
/// header into `wildframe_focus`'s public interface. Forward-
/// declaring the base keeps consumers (`src/run.cpp`, tests) from
/// transitively picking up orchestrator's include surface through
/// this header.

#include <memory>

namespace wildframe::orchestrator {
class PipelineStage;
}  // namespace wildframe::orchestrator

namespace wildframe::focus {

/// Construct a stage that calls `Score` on the `StageContext::preview`
/// populated by `RawStage` and the primary-subject box from
/// `StageContext::detection` populated by `DetectStage`, writing the
/// result into `StageContext::focus` for downstream stages. The
/// returned pointer is move-only and owned by the caller (typically
/// the orchestrator's stage vector).
[[nodiscard]] std::unique_ptr<orchestrator::PipelineStage> MakeFocusStage();

}  // namespace wildframe::focus
