#include "wildframe/detect/detect.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "wildframe/raw/preview_image.hpp"

namespace {

wildframe::raw::PreviewImage MakePreview(int width, int height) {
  const auto pixel_count =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  return wildframe::raw::PreviewImage{
      .width = width,
      .height = height,
      .rgb_bytes = std::vector<std::uint8_t>(pixel_count * 3U, 128U),
  };
}

TEST(DetectStub, ReturnsNoDetectionSentinel) {
  const auto preview = MakePreview(256, 256);
  const wildframe::detect::DetectConfig config;
  const auto result = wildframe::detect::Detect(preview, config);
  EXPECT_FALSE(result.bird_present);
  EXPECT_EQ(result.bird_count, 0);
  EXPECT_TRUE(result.bird_boxes.empty());
  EXPECT_FALSE(result.primary_subject_box.has_value());
  EXPECT_EQ(result.detection_confidence, 0.0F);
}

TEST(DetectStub, IsDeterministicAcrossInputs) {
  // Two different-sized previews should both yield the sentinel
  // unchanged — the stub ignores input shape. This pins the
  // contract M3-* will eventually break when real inference lands.
  const auto small_preview = MakePreview(64, 64);
  const auto large_preview = MakePreview(512, 384);
  const wildframe::detect::DetectConfig config;
  const auto small_result = wildframe::detect::Detect(small_preview, config);
  const auto large_result = wildframe::detect::Detect(large_preview, config);
  EXPECT_EQ(small_result.bird_present, large_result.bird_present);
  EXPECT_EQ(small_result.bird_count, large_result.bird_count);
  EXPECT_EQ(small_result.detection_confidence,
            large_result.detection_confidence);
}

}  // namespace
