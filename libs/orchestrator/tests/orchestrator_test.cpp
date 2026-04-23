#include "wildframe/orchestrator/orchestrator.hpp"

#include <gtest/gtest.h>
#include <spdlog/common.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "wildframe/ingest/image_job.hpp"
#include "wildframe/log/log.hpp"
#include "wildframe/orchestrator/pipeline_stage.hpp"

namespace {

namespace wfo = wildframe::orchestrator;
namespace ingest = wildframe::ingest;

// Orchestrator calls `log::orchestrator.info/debug(...)`; without an
// initialized sink `detail::native` aborts. Every test case runs with
// a silent logger per docs/STYLE.md §4.2 (ctest level).

class OrchestratorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    wildframe::log::Config cfg;
    cfg.enable_stdout = false;
    cfg.level = spdlog::level::warn;
    wildframe::log::Init(cfg);
  }

  void TearDown() override { wildframe::log::Shutdown(); }
};

ingest::ImageJob MakeJob(const std::filesystem::path& path) {
  return ingest::ImageJob{
      .path = path,
      .format = ingest::Format::kCr3,
      .size_bytes = std::nullopt,
      .content_hash = std::nullopt,
  };
}

struct StageCall {
  std::string stage_name;
  std::filesystem::path job_path;
};

class RecordingStage : public wfo::PipelineStage {
 public:
  RecordingStage(std::string name, std::vector<StageCall>* log)
      : name_(std::move(name)), log_(log) {}

  [[nodiscard]] std::string_view Name() const noexcept override {
    return name_;
  }

  wfo::StageResult Process(wfo::StageContext& ctx) override {
    log_->push_back(StageCall{.stage_name = name_, .job_path = ctx.job.path});
    return {};
  }

 private:
  std::string name_;
  std::vector<StageCall>* log_;
};

class ThrowingStage : public wfo::PipelineStage {
 public:
  [[nodiscard]] std::string_view Name() const noexcept override {
    return "throwing";
  }

  wfo::StageResult Process(wfo::StageContext& /*ctx*/) override {
    throw std::runtime_error("stage failed");
  }
};

// --- Run mechanics --------------------------------------------------------

TEST_F(OrchestratorTest, EmptyJobsAndEmptyStagesCompletesCleanly) {
  std::vector<std::unique_ptr<wfo::PipelineStage>> stages;
  wfo::Orchestrator orch(std::move(stages), std::filesystem::path{"/tmp/mf"});

  const auto result = orch.Run({});
  EXPECT_EQ(result.status, wfo::RunStatus::kCompleted);
  EXPECT_EQ(result.jobs_total, 0U);
  EXPECT_EQ(result.jobs_completed, 0U);
}

TEST_F(OrchestratorTest, EmptyStageListStillCountsJobs) {
  std::vector<std::unique_ptr<wfo::PipelineStage>> stages;
  wfo::Orchestrator orch(std::move(stages), std::filesystem::path{"/tmp/mf"});

  const std::vector<ingest::ImageJob> jobs = {
      MakeJob("/tmp/a.CR3"),
      MakeJob("/tmp/b.CR3"),
  };

  const auto result = orch.Run(jobs);
  EXPECT_EQ(result.status, wfo::RunStatus::kCompleted);
  EXPECT_EQ(result.jobs_total, 2U);
  EXPECT_EQ(result.jobs_completed, 2U);
}

TEST_F(OrchestratorTest, StagesInvokedInRegistrationOrderForEveryJob) {
  std::vector<StageCall> calls;
  std::vector<std::unique_ptr<wfo::PipelineStage>> stages;
  stages.push_back(std::make_unique<RecordingStage>("first", &calls));
  stages.push_back(std::make_unique<RecordingStage>("second", &calls));
  stages.push_back(std::make_unique<RecordingStage>("third", &calls));

  wfo::Orchestrator orch(std::move(stages), std::filesystem::path{"/tmp/mf"});

  const std::vector<ingest::ImageJob> jobs = {
      MakeJob("/tmp/a.CR3"),
      MakeJob("/tmp/b.CR3"),
  };
  (void)orch.Run(jobs);

  // Six calls total: 2 jobs × 3 stages, each job's stages in order
  // before the next job starts. `.at()` instead of `[]` satisfies
  // `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access`
  // and turns any indexing mistake into an out_of_range throw.
  ASSERT_EQ(calls.size(), 6U);
  EXPECT_EQ(calls.at(0).stage_name, "first");
  EXPECT_EQ(calls.at(0).job_path, std::filesystem::path{"/tmp/a.CR3"});
  EXPECT_EQ(calls.at(1).stage_name, "second");
  EXPECT_EQ(calls.at(1).job_path, std::filesystem::path{"/tmp/a.CR3"});
  EXPECT_EQ(calls.at(2).stage_name, "third");
  EXPECT_EQ(calls.at(2).job_path, std::filesystem::path{"/tmp/a.CR3"});
  EXPECT_EQ(calls.at(3).stage_name, "first");
  EXPECT_EQ(calls.at(3).job_path, std::filesystem::path{"/tmp/b.CR3"});
  EXPECT_EQ(calls.at(4).stage_name, "second");
  EXPECT_EQ(calls.at(4).job_path, std::filesystem::path{"/tmp/b.CR3"});
  EXPECT_EQ(calls.at(5).stage_name, "third");
  EXPECT_EQ(calls.at(5).job_path, std::filesystem::path{"/tmp/b.CR3"});
}

TEST_F(OrchestratorTest, ProgressCallbackFiresOncePerCompletedJob) {
  std::vector<wfo::ProgressUpdate> updates;
  std::vector<std::unique_ptr<wfo::PipelineStage>> stages;

  wfo::Orchestrator orch(std::move(stages), std::filesystem::path{"/tmp/mf"},
                         [&](wfo::ProgressUpdate update) {
                           updates.push_back(std::move(update));
                         });

  const std::vector<ingest::ImageJob> jobs = {
      MakeJob("/tmp/a.CR3"),
      MakeJob("/tmp/b.CR3"),
      MakeJob("/tmp/c.CR3"),
  };
  (void)orch.Run(jobs);

  ASSERT_EQ(updates.size(), 3U);
  EXPECT_EQ(updates.at(0).completed, 1U);
  EXPECT_EQ(updates.at(0).total, 3U);
  EXPECT_EQ(updates.at(0).current_job, std::filesystem::path{"/tmp/a.CR3"});
  EXPECT_EQ(updates.at(1).completed, 2U);
  EXPECT_EQ(updates.at(1).current_job, std::filesystem::path{"/tmp/b.CR3"});
  EXPECT_EQ(updates.at(2).completed, 3U);
  EXPECT_EQ(updates.at(2).current_job, std::filesystem::path{"/tmp/c.CR3"});
}

TEST_F(OrchestratorTest, NoProgressCallbackIsSafe) {
  std::vector<std::unique_ptr<wfo::PipelineStage>> stages;
  wfo::Orchestrator orch(std::move(stages), std::filesystem::path{"/tmp/mf"});

  const std::vector<ingest::ImageJob> jobs = {MakeJob("/tmp/only.CR3")};
  const auto result = orch.Run(jobs);

  EXPECT_EQ(result.jobs_completed, 1U);
}

// --- Error propagation (M6-04 error isolation is future work) -------------

TEST_F(OrchestratorTest, StageExceptionPropagatesAndAbortsBatch) {
  std::vector<StageCall> calls;
  std::vector<std::unique_ptr<wfo::PipelineStage>> stages;
  stages.push_back(std::make_unique<RecordingStage>("before", &calls));
  stages.push_back(std::make_unique<ThrowingStage>());
  stages.push_back(std::make_unique<RecordingStage>("after", &calls));

  wfo::Orchestrator orch(std::move(stages), std::filesystem::path{"/tmp/mf"});

  const std::vector<ingest::ImageJob> jobs = {
      MakeJob("/tmp/a.CR3"),
      MakeJob("/tmp/b.CR3"),
  };

  EXPECT_THROW({ (void)orch.Run(jobs); }, std::runtime_error);

  // The first stage for job "a" ran; the throwing stage aborted the
  // batch before the trailing stage or the second job's pipeline.
  ASSERT_EQ(calls.size(), 1U);
  EXPECT_EQ(calls.at(0).stage_name, "before");
  EXPECT_EQ(calls.at(0).job_path, std::filesystem::path{"/tmp/a.CR3"});
}

// --- Accessors ------------------------------------------------------------

TEST_F(OrchestratorTest, ManifestDirAccessorReturnsConstructorInput) {
  std::vector<std::unique_ptr<wfo::PipelineStage>> stages;
  const std::filesystem::path manifest_dir{"/tmp/wildframe-manifests"};
  const wfo::Orchestrator orch(std::move(stages), manifest_dir);

  EXPECT_EQ(orch.manifest_dir(), manifest_dir);
}

}  // namespace
