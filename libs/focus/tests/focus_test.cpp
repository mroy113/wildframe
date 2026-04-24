#include "wildframe/focus/focus.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "wildframe/detect/bbox.hpp"
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

TEST(FocusStub, ReturnsZeroQualitySentinel) {
  const auto preview = MakePreview(256, 256);
  const wildframe::detect::BBox box{
      .x = 0.25F, .y = 0.25F, .width = 0.5F, .height = 0.5F};
  const wildframe::focus::FocusConfig config;
  const auto result = wildframe::focus::Score(preview, box, config);
  EXPECT_EQ(result.focus_score, 0.0F);
  EXPECT_EQ(result.motion_blur_score, 0.0F);
  EXPECT_EQ(result.subject_size_percent, 0.0F);
  EXPECT_EQ(result.keeper_score, 0.0F);
  EXPECT_FALSE(result.edge_clipped.at(wildframe::focus::kEdgeTop));
  EXPECT_FALSE(result.edge_clipped.at(wildframe::focus::kEdgeBottom));
  EXPECT_FALSE(result.edge_clipped.at(wildframe::focus::kEdgeLeft));
  EXPECT_FALSE(result.edge_clipped.at(wildframe::focus::kEdgeRight));
}

TEST(FocusStub, AcceptsNullPrimarySubject) {
  // Per `wildframe_focus::Score` docstring, an upstream detector that
  // found no bird passes `std::nullopt`; the stub still returns the
  // sentinel unchanged.
  const auto preview = MakePreview(64, 64);
  const std::optional<wildframe::detect::BBox> primary_subject;
  const wildframe::focus::FocusConfig config;
  const auto result = wildframe::focus::Score(preview, primary_subject, config);
  EXPECT_EQ(result.focus_score, 0.0F);
  EXPECT_EQ(result.keeper_score, 0.0F);
}

TEST(FocusStub, IsDeterministicAcrossInputs) {
  const auto small_preview = MakePreview(64, 64);
  const auto large_preview = MakePreview(512, 384);
  const wildframe::detect::BBox box{
      .x = 0.1F, .y = 0.1F, .width = 0.2F, .height = 0.2F};
  const wildframe::focus::FocusConfig config;
  const auto small_result = wildframe::focus::Score(small_preview, box, config);
  const auto large_result = wildframe::focus::Score(large_preview, box, config);
  EXPECT_EQ(small_result.focus_score, large_result.focus_score);
  EXPECT_EQ(small_result.motion_blur_score, large_result.motion_blur_score);
  EXPECT_EQ(small_result.subject_size_percent,
            large_result.subject_size_percent);
  EXPECT_EQ(small_result.keeper_score, large_result.keeper_score);
  EXPECT_EQ(small_result.edge_clipped, large_result.edge_clipped);
}

}  // namespace
