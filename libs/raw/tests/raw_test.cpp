#include "wildframe/raw/raw.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <system_error>

#include "wildframe/detect/bbox.hpp"
#include "wildframe/raw/preview_image.hpp"

namespace {

constexpr std::uint8_t kExpectedStubChannelValue = 128;
constexpr int kExpectedStubSide = 256;

bool AllChannelsMatch(const wildframe::raw::PreviewImage& preview,
                      std::uint8_t expected) {
  return std::ranges::all_of(preview.rgb_bytes, [expected](std::uint8_t byte) {
    return byte == expected;
  });
}

TEST(ExtractPreviewStub, ReturnsDeterministicGrayBuffer) {
  const auto preview =
      wildframe::raw::ExtractPreview("/nonexistent/example.CR3");
  EXPECT_EQ(preview.width, kExpectedStubSide);
  EXPECT_EQ(preview.height, kExpectedStubSide);
  const auto expected_bytes = static_cast<std::size_t>(kExpectedStubSide) *
                              static_cast<std::size_t>(kExpectedStubSide) * 3U;
  EXPECT_EQ(preview.rgb_bytes.size(), expected_bytes);
  EXPECT_TRUE(AllChannelsMatch(preview, kExpectedStubChannelValue));
}

TEST(ExtractPreviewStub, IsByteIdenticalAcrossCalls) {
  const auto first = wildframe::raw::ExtractPreview("/a.CR3");
  const auto second = wildframe::raw::ExtractPreview("/b.CR3");
  EXPECT_EQ(first.width, second.width);
  EXPECT_EQ(first.height, second.height);
  EXPECT_EQ(first.rgb_bytes, second.rgb_bytes);
}

TEST(DecodeCropStub, SizesBufferToBBox) {
  wildframe::detect::BBox bbox;
  bbox.width = 64.0F;
  bbox.height = 32.0F;
  const auto cropped = wildframe::raw::DecodeCrop("/a.CR3", bbox);
  EXPECT_EQ(cropped.width, 64);
  EXPECT_EQ(cropped.height, 32);
  EXPECT_EQ(cropped.rgb_bytes.size(), 64U * 32U * 3U);
  EXPECT_TRUE(AllChannelsMatch(cropped, kExpectedStubChannelValue));
}

TEST(DecodeCropStub, ClampsDegenerateBBoxToOnePixel) {
  const wildframe::detect::BBox empty_bbox;
  const auto cropped = wildframe::raw::DecodeCrop("/a.CR3", empty_bbox);
  EXPECT_EQ(cropped.width, 1);
  EXPECT_EQ(cropped.height, 1);
  EXPECT_EQ(cropped.rgb_bytes.size(), 3U);
}

TEST(ExtractPreviewStub, AcceptsRealFixturePaths) {
  const std::filesystem::path fixtures{WILDFRAME_FIXTURES_DIR};
  if (!std::filesystem::exists(fixtures)) {
    GTEST_SKIP() << "S0-12 fixtures unavailable: " << fixtures;
  }
  std::error_code iter_error;
  bool exercised_any = false;
  for (const auto& entry :
       std::filesystem::directory_iterator(fixtures, iter_error)) {
    if (iter_error || !entry.is_regular_file()) {
      continue;
    }
    const auto preview = wildframe::raw::ExtractPreview(entry.path());
    EXPECT_FALSE(preview.rgb_bytes.empty());
    exercised_any = true;
  }
  EXPECT_TRUE(exercised_any)
      << "fixtures directory exists but contains no files: " << fixtures;
}

}  // namespace
