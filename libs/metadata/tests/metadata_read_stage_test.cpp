#include "wildframe/metadata/metadata_read_stage.hpp"

#include <gtest/gtest.h>
#include <spdlog/common.h>

#include <filesystem>
#include <memory>

#include "wildframe/ingest/image_job.hpp"
#include "wildframe/log/log.hpp"
#include "wildframe/orchestrator/pipeline_stage.hpp"

namespace {

class MetadataReadStageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    wildframe::log::Config cfg;
    cfg.enable_stdout = false;
    cfg.level = spdlog::level::warn;
    wildframe::log::Init(cfg);
  }

  void TearDown() override { wildframe::log::Shutdown(); }

  static wildframe::orchestrator::StageContext MakeContext(
      const std::filesystem::path& path) {
    return wildframe::orchestrator::StageContext{
        .job = wildframe::ingest::ImageJob{.path = path},
    };
  }
};

TEST_F(MetadataReadStageTest, PopulatesExifSentinelOnStageContext) {
  auto stage = wildframe::metadata::MakeMetadataReadStage();
  ASSERT_NE(stage, nullptr);
  EXPECT_EQ(stage->Name(), "metadata_read");

  auto context = MakeContext("/fixtures/IMG_0001.CR3");
  ASSERT_FALSE(context.exif.has_value());

  (void)stage->Process(context);

  ASSERT_TRUE(context.exif.has_value());
  // Pair the ASSERT_TRUE with a plain `if` per docs/STYLE.md §2.13 —
  // bugprone-unchecked-optional-access cannot model gtest's
  // aborts-caller macro contract.
  if (context.exif.has_value()) {
    EXPECT_EQ(context.exif->file_path, "/fixtures/IMG_0001.CR3");
    EXPECT_EQ(context.exif->file_name, "IMG_0001.CR3");
    EXPECT_FALSE(context.exif->capture_datetime.has_value());
    EXPECT_FALSE(context.exif->camera_model.has_value());
    EXPECT_FALSE(context.exif->focal_length.has_value());
    EXPECT_FALSE(context.exif->iso.has_value());
  }
}

TEST_F(MetadataReadStageTest, TracksFilesystemFieldsPerJob) {
  auto stage = wildframe::metadata::MakeMetadataReadStage();
  ASSERT_NE(stage, nullptr);

  auto first = MakeContext("/fixtures/a.CR3");
  auto second = MakeContext("/fixtures/deeper/b.CR3");
  (void)stage->Process(first);
  (void)stage->Process(second);

  ASSERT_TRUE(first.exif.has_value());
  ASSERT_TRUE(second.exif.has_value());
  if (first.exif.has_value() && second.exif.has_value()) {
    EXPECT_EQ(first.exif->file_name, "a.CR3");
    EXPECT_EQ(second.exif->file_name, "b.CR3");
    EXPECT_NE(first.exif->file_path, second.exif->file_path);
  }
}

}  // namespace
