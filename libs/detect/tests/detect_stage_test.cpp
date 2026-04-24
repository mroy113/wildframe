#include "wildframe/detect/detect_stage.hpp"

#include <gtest/gtest.h>
#include <spdlog/common.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "wildframe/ingest/image_job.hpp"
#include "wildframe/log/log.hpp"
#include "wildframe/orchestrator/pipeline_stage.hpp"
#include "wildframe/raw/preview_image.hpp"

namespace {

class DetectStageTest : public ::testing::Test {
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

TEST_F(DetectStageTest, PopulatesDetectionSentinelOnStageContext) {
  auto stage = wildframe::detect::MakeDetectStage();
  ASSERT_NE(stage, nullptr);
  EXPECT_EQ(stage->Name(), "detect");

  auto context = MakeContextWithPreview();
  ASSERT_FALSE(context.detection.has_value());

  (void)stage->Process(context);

  ASSERT_TRUE(context.detection.has_value());
  // Pair the ASSERT_TRUE with a plain `if` per docs/STYLE.md §2.13 —
  // bugprone-unchecked-optional-access cannot model gtest's
  // aborts-caller macro contract.
  if (context.detection.has_value()) {
    EXPECT_FALSE(context.detection->bird_present);
    EXPECT_EQ(context.detection->bird_count, 0);
    EXPECT_TRUE(context.detection->bird_boxes.empty());
    EXPECT_FALSE(context.detection->primary_subject_box.has_value());
    EXPECT_EQ(context.detection->detection_confidence, 0.0F);
  }
}

TEST_F(DetectStageTest, IsRunnableRepeatedly) {
  auto stage = wildframe::detect::MakeDetectStage();
  ASSERT_NE(stage, nullptr);

  auto first = MakeContextWithPreview();
  auto second = MakeContextWithPreview();
  (void)stage->Process(first);
  (void)stage->Process(second);

  ASSERT_TRUE(first.detection.has_value());
  ASSERT_TRUE(second.detection.has_value());
  if (first.detection.has_value() && second.detection.has_value()) {
    EXPECT_EQ(first.detection->bird_present, second.detection->bird_present);
    EXPECT_EQ(first.detection->bird_count, second.detection->bird_count);
  }
}

}  // namespace
