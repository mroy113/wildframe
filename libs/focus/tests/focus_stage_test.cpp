#include "wildframe/focus/focus_stage.hpp"

#include <gtest/gtest.h>
#include <spdlog/common.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "wildframe/detect/detect.hpp"
#include "wildframe/focus/focus.hpp"
#include "wildframe/ingest/image_job.hpp"
#include "wildframe/log/log.hpp"
#include "wildframe/orchestrator/pipeline_stage.hpp"
#include "wildframe/raw/preview_image.hpp"

namespace {

class FocusStageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    wildframe::log::Config cfg;
    cfg.enable_stdout = false;
    cfg.level = spdlog::level::warn;
    wildframe::log::Init(cfg);
  }

  void TearDown() override { wildframe::log::Shutdown(); }

  static wildframe::orchestrator::StageContext MakeContextWithPreview() {
    wildframe::orchestrator::StageContext context{
        .job = wildframe::ingest::ImageJob{.path = "/irrelevant.CR3"},
    };
    constexpr std::size_t kPreviewByteCount = 8UL * 8UL * 3UL;
    context.preview = wildframe::raw::PreviewImage{
        .width = 8,
        .height = 8,
        .rgb_bytes = std::vector<std::uint8_t>(kPreviewByteCount, 128U),
    };
    return context;
  }
};

TEST_F(FocusStageTest, PopulatesFocusSentinelOnStageContext) {
  auto stage = wildframe::focus::MakeFocusStage();
  ASSERT_NE(stage, nullptr);
  EXPECT_EQ(stage->Name(), "focus");

  auto context = MakeContextWithPreview();
  // A realistic mid-pipeline context: the detector ran and reported
  // "no bird". FocusStage must still populate the sentinel so
  // MetadataWriteStage sees a value to serialize.
  context.detection = wildframe::detect::DetectionResult{};
  ASSERT_FALSE(context.focus.has_value());

  (void)stage->Process(context);

  ASSERT_TRUE(context.focus.has_value());
  // Pair the ASSERT_TRUE with a plain `if` per docs/STYLE.md §2.13 —
  // bugprone-unchecked-optional-access cannot model gtest's
  // aborts-caller macro contract.
  if (context.focus.has_value()) {
    EXPECT_EQ(context.focus->focus_score, 0.0F);
    EXPECT_EQ(context.focus->motion_blur_score, 0.0F);
    EXPECT_EQ(context.focus->subject_size_percent, 0.0F);
    EXPECT_EQ(context.focus->keeper_score, 0.0F);
    EXPECT_FALSE(context.focus->edge_clipped.at(wildframe::focus::kEdgeTop));
    EXPECT_FALSE(context.focus->edge_clipped.at(wildframe::focus::kEdgeBottom));
    EXPECT_FALSE(context.focus->edge_clipped.at(wildframe::focus::kEdgeLeft));
    EXPECT_FALSE(context.focus->edge_clipped.at(wildframe::focus::kEdgeRight));
  }
}

TEST_F(FocusStageTest, RunsWithoutDetectionResult) {
  // If DetectStage never ran (an orchestrator bug the stage contract
  // cannot prevent today), FocusStage still writes the sentinel
  // rather than crashing on a nullopt dereference.
  auto stage = wildframe::focus::MakeFocusStage();
  ASSERT_NE(stage, nullptr);

  auto context = MakeContextWithPreview();
  ASSERT_FALSE(context.detection.has_value());

  (void)stage->Process(context);

  ASSERT_TRUE(context.focus.has_value());
  if (context.focus.has_value()) {
    EXPECT_EQ(context.focus->keeper_score, 0.0F);
  }
}

TEST_F(FocusStageTest, IsRunnableRepeatedly) {
  auto stage = wildframe::focus::MakeFocusStage();
  ASSERT_NE(stage, nullptr);

  auto first = MakeContextWithPreview();
  auto second = MakeContextWithPreview();
  first.detection = wildframe::detect::DetectionResult{};
  second.detection = wildframe::detect::DetectionResult{};
  (void)stage->Process(first);
  (void)stage->Process(second);

  ASSERT_TRUE(first.focus.has_value());
  ASSERT_TRUE(second.focus.has_value());
  if (first.focus.has_value() && second.focus.has_value()) {
    EXPECT_EQ(first.focus->keeper_score, second.focus->keeper_score);
    EXPECT_EQ(first.focus->focus_score, second.focus->focus_score);
  }
}

}  // namespace
