#include "wildframe/raw/raw_stage.hpp"

#include <gtest/gtest.h>
#include <spdlog/common.h>

#include <memory>

#include "wildframe/ingest/image_job.hpp"
#include "wildframe/log/log.hpp"
#include "wildframe/orchestrator/pipeline_stage.hpp"

namespace {

class RawStageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    wildframe::log::Config cfg;
    cfg.enable_stdout = false;
    cfg.level = spdlog::level::warn;
    wildframe::log::Init(cfg);
  }

  void TearDown() override { wildframe::log::Shutdown(); }
};

TEST_F(RawStageTest, PopulatesPreviewOnStageContext) {
  auto stage = wildframe::raw::MakeRawStage();
  ASSERT_NE(stage, nullptr);
  EXPECT_EQ(stage->Name(), "raw");

  wildframe::orchestrator::StageContext ctx{
      .job = wildframe::ingest::ImageJob{.path = "/irrelevant.CR3"},
  };
  ASSERT_FALSE(ctx.preview.has_value());

  (void)stage->Process(ctx);

  ASSERT_TRUE(ctx.preview.has_value());
  // Pair the ASSERT_TRUE with a plain `if` per docs/STYLE.md §2.13 —
  // bugprone-unchecked-optional-access cannot model gtest's
  // aborts-caller macro contract.
  if (ctx.preview.has_value()) {
    EXPECT_GT(ctx.preview->width, 0);
    EXPECT_GT(ctx.preview->height, 0);
    EXPECT_FALSE(ctx.preview->rgb_bytes.empty());
  }
}

TEST_F(RawStageTest, IsRunnableRepeatedly) {
  auto stage = wildframe::raw::MakeRawStage();
  ASSERT_NE(stage, nullptr);

  wildframe::orchestrator::StageContext first{
      .job = wildframe::ingest::ImageJob{.path = "/one.CR3"},
  };
  wildframe::orchestrator::StageContext second{
      .job = wildframe::ingest::ImageJob{.path = "/two.CR3"},
  };
  (void)stage->Process(first);
  (void)stage->Process(second);

  ASSERT_TRUE(first.preview.has_value());
  ASSERT_TRUE(second.preview.has_value());
  if (first.preview.has_value() && second.preview.has_value()) {
    EXPECT_EQ(first.preview->rgb_bytes, second.preview->rgb_bytes);
  }
}

}  // namespace
