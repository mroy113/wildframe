#pragma once

/// \file
/// Public API for `wildframe_raw` (Module 2, FR-2, handoff §10).
///
/// Two operations, both operating on a RAW file's filesystem path:
///   - `ExtractPreview` — fast path, returns the largest embedded
///     JPEG preview decoded to RGB. Drives every downstream stage in
///     the MVP (detect, focus, metadata).
///   - `DecodeCrop` — slow fallback used only when the preview's
///     resolution is insufficient for focus scoring on a small
///     subject (FR-4, handoff §15 technical risks).
///
/// Both land as stubs in TB-03 (deterministic gray buffer, no LibRaw
/// link) and get their real LibRaw implementations in M2-01 / M2-02
/// without changing these signatures. A LibRaw exception raised by
/// the real impl will be translated to `wildframe::raw::RawDecodeError`
/// at this same boundary (M2-03, `docs/STYLE.md` §3.1).

#include <filesystem>

#include "wildframe/detect/bbox.hpp"
#include "wildframe/raw/preview_image.hpp"

namespace wildframe::raw {

/// Extract the largest embedded JPEG preview from `raw_path` and
/// return it decoded to packed 8-bit RGB. Stub returns a deterministic
/// 256×256 mid-gray buffer regardless of input; M2-01 replaces with
/// LibRaw.
///
/// \throws RawDecodeError on decode failure (once M2-03 lands).
[[nodiscard]] PreviewImage ExtractPreview(
    const std::filesystem::path& raw_path);

/// Decode the region described by `crop_region` at full RAW
/// resolution. Used only when the preview is too low-resolution for
/// focus metrics on a small subject. Stub returns a gray buffer sized
/// to the crop; M2-02 replaces with LibRaw. M2-02 may revise the
/// return type (`CroppedImage`) and pass mode; the revision lands
/// with its own backlog update per `CONTRIBUTING.md`.
///
/// \throws RawDecodeError on decode failure (once M2-03 lands).
[[nodiscard]] PreviewImage DecodeCrop(const std::filesystem::path& raw_path,
                                      const detect::BBox& crop_region);

}  // namespace wildframe::raw
