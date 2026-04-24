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

/// Coordinate system and type choice.
///
/// Fields are **normalized fractions of the preview image's
/// dimensions** in `[0.0, 1.0]`, matching the XMP schema in
/// `docs/METADATA.md` §3.1 (`wildframe:bird_boxes` entries are
/// `Real` with range `[0.0, 1.0]`). Origin is the preview's
/// top-left. Keeping the in-memory representation aligned with the
/// on-disk representation eliminates a resolution-dependent
/// conversion at the metadata boundary.
///
/// The fields are `float` rather than `int` / `std::size_t`
/// because:
/// - The canonical range is `[0.0, 1.0]`, a continuous domain.
///   Integer types cannot represent it without a scale factor that
///   would have to be agreed with every consumer.
/// - YOLOv11 / MegaDetector both emit float detections; converting
///   to an integer at the detector boundary throws away sub-pixel
///   precision that the small-subject crop fallback (FR-4, M4-06)
///   later wants when it reverses the normalization.
/// - `std::size_t` specifically would forbid the tiny negative
///   values that appear in intermediate math (letterbox reverse
///   before clamping) and crash a signed-to-unsigned conversion
///   at the detector edge.
///
/// M3-03 is responsible for clamping to `[0.0, 1.0]` before
/// emission; consumers downstream of the detect stage may treat
/// every field as in-range.

namespace wildframe::detect {

struct BBox {
  float x = 0.0F;
  float y = 0.0F;
  float width = 0.0F;
  float height = 0.0F;
};

/// Rectangle area as a fraction of preview area (since both
/// dimensions are normalized). Used as the primary-subject
/// selection tiebreak (FR-3). Assumes non-negative extents, which
/// M3-03 guarantees.
[[nodiscard]] constexpr float Area(const BBox& bbox) noexcept {
  return bbox.width * bbox.height;
}

}  // namespace wildframe::detect
