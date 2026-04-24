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

/// Mid-gray 8-bit channel value. Deterministic so TB-09 can assert
/// byte-level equality across runs.
constexpr std::uint8_t kStubChannelValue = 128;

[[nodiscard]] PreviewImage MakeGrayBuffer(int width, int height) {
  const auto pixel_count =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  return PreviewImage{
      .width = width,
      .height = height,
      .rgb_bytes =
          std::vector<std::uint8_t>(pixel_count * 3U, kStubChannelValue),
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
  // at full RAW resolution. The stub sizes a gray buffer to the
  // requested crop so callers can exercise shape handling; clamping
  // to a 1×1 minimum avoids a degenerate empty buffer when the bbox
  // is zero-sized (e.g. a default-constructed `BBox`).
  const int width = std::max(1, static_cast<int>(crop_region.width));
  const int height = std::max(1, static_cast<int>(crop_region.height));
  return MakeGrayBuffer(width, height);
}

}  // namespace wildframe::raw
