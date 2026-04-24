#pragma once

/// \file
/// `MakeRawStage` — factory for the orchestrator's per-image raw
/// decode step (TB-03, handoff §12).
///
/// The concrete `RawStage` class lives in the anonymous namespace of
/// `raw_stage.cpp` so it can inherit from `orchestrator::PipelineStage`
/// without pulling the full base header into `wildframe_raw`'s public
/// interface — the forward declaration below is enough for the
/// `std::unique_ptr<...>` return type. Consumers
/// (`src/run.cpp`, tests) include `pipeline_stage.hpp` themselves
/// when they dereference the pointer.

#include <memory>

namespace wildframe::orchestrator {
class PipelineStage;
}  // namespace wildframe::orchestrator

namespace wildframe::raw {

/// Construct a stage that runs `ExtractPreview` on the current
/// `StageContext::job.path` and writes the result into
/// `StageContext::preview` for downstream stages. The returned
/// pointer is move-only and owned by the caller (typically the
/// orchestrator's stage vector).
[[nodiscard]] std::unique_ptr<orchestrator::PipelineStage> MakeRawStage();

}  // namespace wildframe::raw
