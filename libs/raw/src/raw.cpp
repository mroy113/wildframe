#include "wildframe/raw/raw.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "wildframe/detect/bbox.hpp"
#include "wildframe/raw/preview_image.hpp"

namespace wildframe::raw {

namespace {

/// Stub preview side length, in pixels. M2-01 replaces this with
/// whatever LibRaw returns from the embedded JPEG.
constexpr int kStubPreviewSideLength = 256;

/// Simulated "full-res RAW" long edge the stub `DecodeCrop` uses to
/// turn a normalized `BBox` (see `docs/METADATA.md` §3.1) into a
/// concrete pixel size. M2-02 replaces this with LibRaw's actual
/// full-resolution dimensions.
constexpr int kStubFullResolutionSideLength = 256;

/// Channels per pixel in a packed 8-bit RGB buffer. Named so the
/// `* kRgbChannelsPerPixel` factor in `MakeGrayBuffer` reads as
/// "three bytes per pixel" rather than a bare `3`.
constexpr std::size_t kRgbChannelsPerPixel = 3U;

/// Mid-gray 8-bit channel value. Deterministic so TB-09 can assert
/// byte-level equality across runs.
constexpr std::uint8_t kStubChannelValue = 128;

[[nodiscard]] PreviewImage MakeGrayBuffer(int width, int height) {
  const auto pixel_count =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  return PreviewImage{
      .width = width,
      .height = height,
      .rgb_bytes = std::vector<std::uint8_t>(pixel_count * kRgbChannelsPerPixel,
                                             kStubChannelValue),
  };
}

}  // namespace

PreviewImage ExtractPreview(const std::filesystem::path& /*raw_path*/) {
  // M2-01 will open the file via LibRaw and return the largest
  // embedded JPEG decoded to RGB. Until then, the stub returns a
  // deterministic buffer whose shape (256×256) is documented for
  // TB-09 assertions.
  return MakeGrayBuffer(kStubPreviewSideLength, kStubPreviewSideLength);
}

PreviewImage DecodeCrop(const std::filesystem::path& /*raw_path*/,
                        const detect::BBox& crop_region) {
  // M2-02 will open the file via LibRaw and decode the crop region
  // at full RAW resolution. The stub scales the normalized crop
  // (`BBox` fields are `[0.0, 1.0]`, see `docs/METADATA.md` §3.1)
  // by a simulated full-res side length so callers exercise the
  // shape flow; clamping to a 1×1 minimum avoids a degenerate empty
  // buffer for a default-constructed `BBox`.
  const auto scale = static_cast<float>(kStubFullResolutionSideLength);
  const int width = std::max(1, static_cast<int>(crop_region.width * scale));
  const int height = std::max(1, static_cast<int>(crop_region.height * scale));
  return MakeGrayBuffer(width, height);
}

}  // namespace wildframe::raw
