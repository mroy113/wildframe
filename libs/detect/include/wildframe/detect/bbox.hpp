#pragma once

/// \file
/// `BBox` — axis-aligned rectangle in preview-pixel coordinates
/// (TB-03 early introduction, consumed by TB-04 / M3-*).
///
/// Produced by `wildframe_detect::Detect` and consumed by
/// `wildframe_raw::DecodeCrop` (the focus small-subject fallback,
/// FR-4) and `wildframe_focus::Score` (the primary-subject crop).
///
/// The type lands in TB-03 rather than the TB-04 task that introduces
/// the rest of the detect API; see the matching plan change
/// (`plan: TB-03 introduces BBox early`) in the Git history for the
/// rationale. The plan's original wording put `Area()` on the struct
/// as a member; shipping it as a free function keeps `BBox` a pure
/// aggregate consistent with `ImageJob` / `FocusResult` /
/// `DetectionResult` and sidesteps
/// `misc-non-private-member-variables-in-classes` without a NOLINT
/// or a project-wide clang-tidy override. Same call site ergonomics
/// as a member (`Area(subject)` vs `subject.Area()`), zero semantic
/// change.

namespace wildframe::detect {

/// Axis-aligned rectangle in preview-pixel coordinates. Origin is at
/// the preview's top-left; `x`/`y` locate the upper-left corner;
/// `width` and `height` are extents. All four fields are independent
/// and unvalidated at construction — the detector is responsible for
/// clamping to preview bounds before emission (M3-03 will do this).
struct BBox {
  float x = 0.0F;
  float y = 0.0F;
  float width = 0.0F;
  float height = 0.0F;
};

/// Rectangle area in pixel². Used as the primary-subject selection
/// tiebreak (FR-3). Assumes non-negative extents; the detector
/// guarantees that.
[[nodiscard]] constexpr float Area(const BBox& bbox) noexcept {
  return bbox.width * bbox.height;
}

}  // namespace wildframe::detect
