#include "wildframe/orchestrator/orchestrator.hpp"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "wildframe/ingest/image_job.hpp"
#include "wildframe/log/log.hpp"
#include "wildframe/orchestrator/pipeline_stage.hpp"

namespace wildframe::orchestrator {

Orchestrator::Orchestrator(std::vector<std::unique_ptr<PipelineStage>> stages,
                           std::filesystem::path manifest_dir,
                           ProgressCallback on_progress)
    : stages_(std::move(stages)),
      manifest_dir_(std::move(manifest_dir)),
      on_progress_(std::move(on_progress)) {}

RunResult Orchestrator::Run(std::span<const ingest::ImageJob> jobs) {
  RunResult result{
      .status = RunStatus::kCompleted,
      .jobs_total = jobs.size(),
      .jobs_completed = 0,
  };

  log::orchestrator.info("batch started: {} jobs, {} stages", result.jobs_total,
                         stages_.size());

  for (const auto& job : jobs) {
    StageContext ctx{.job = job};
    for (const auto& stage : stages_) {
      WF_DEBUG(log::orchestrator, "stage {} running for {}", stage->Name(),
               job.path.string());
      (void)stage->Process(ctx);
    }
    ++result.jobs_completed;
    if (on_progress_) {
      on_progress_(ProgressUpdate{
          .completed = result.jobs_completed,
          .total = result.jobs_total,
          .current_job = job.path,
      });
    }
  }

  log::orchestrator.info("batch finished: {}/{} jobs completed",
                         result.jobs_completed, result.jobs_total);
  return result;
}

}  // namespace wildframe::orchestrator
