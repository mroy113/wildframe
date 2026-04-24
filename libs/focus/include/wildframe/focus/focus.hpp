#pragma once

/// \file
/// Public API for `wildframe_focus` (Module 4, FR-4, handoff §10).
///
/// The focus stage consumes a decoded preview plus the detector's
/// primary-subject box and emits per-image quality scores plus the
/// composite keeper score used by the review UI. TB-05 introduces
/// the public types and the stub entry point that returns a
/// deterministic "no quality signal" sentinel; M4-01..M4-07 wire the
/// real OpenCV-backed scoring behind the same signatures without
/// changing this header.
///
/// Exception policy (`docs/STYLE.md` §3): the real implementation will
/// translate exceptions raised by OpenCV (`cv::Exception`) at this
/// boundary to `wildframe::focus::FocusError`. The TB-05 stub never
/// throws.

#include <array>
#include <cstddef>
#include <optional>

#include "wildframe/detect/bbox.hpp"
#include "wildframe/raw/preview_image.hpp"

namespace wildframe::focus {

/// Runtime-tunable focus knobs. Empty in TB-05 — the real fields
/// (`laplacian_saturation`, keeper-score weights, edge-clipping
/// tolerance) land with M4-02 / M4-04 / M4-05 in the same PR that
/// widens TB-01's TOML parser with the matching `[focus]` keys.
/// Declared now so the stub's signature is stable across the
/// thickening pass (`docs/STYLE.md` §2.11 — the sole caller is the
/// stub stage landing in this same task).
struct FocusConfig {};

/// Index convention for `FocusResult::edge_clipped`, matching the
/// field order in `docs/METADATA.md` §3.2 (`edge_clipping` struct).
/// The on-disk struct is named (top/bottom/left/right); the in-
/// memory array keeps the four flags contiguous without a helper
/// type and without a method on a public-field struct (which would
/// trip `misc-non-private-member-variables-in-classes` per
/// `docs/STYLE.md` §2.15). Plain `constexpr std::size_t` rather than
/// an enum so the values plug straight into `std::array<bool,
/// 4>::at()` without a cast at every call site.
inline constexpr std::size_t kEdgeTop = 0;
inline constexpr std::size_t kEdgeBottom = 1;
inline constexpr std::size_t kEdgeLeft = 2;
inline constexpr std::size_t kEdgeRight = 3;

/// Per-image focus output. Field shape mirrors the AI namespace in
/// `docs/METADATA.md` §3 (`focus_score`, `motion_blur_score`,
/// `subject_size_percent`, `keeper_score`, `edge_clipping`).
///
/// Stub returns every field at its "no quality signal" value — all
/// floats zero, all edge flags false. Consumers that treat
/// `bird_present=false` as the signal to short-circuit focus scoring
/// (per `docs/METADATA.md` §3's "Required if `bird_count > 0`; absent
/// otherwise" rule for the scalar fields, and `keeper_score = 0.0`
/// when `bird_present = False`) therefore see the correct shape
/// unchanged when M4-* lands.
///
/// Plain aggregate (`docs/STYLE.md` §2.1): bare `snake_case` fields,
/// Rule of Zero.
struct FocusResult {
  float focus_score = 0.0F;
  float motion_blur_score = 0.0F;
  float subject_size_percent = 0.0F;
  float keeper_score = 0.0F;
  std::array<bool, 4> edge_clipped = {false, false, false, false};
};

/// Run focus scoring against one decoded preview and optional
/// primary-subject box. Stub returns the "no quality signal"
/// sentinel described on `FocusResult`; M4-06 replaces the body with
/// a real OpenCV-backed computation under the same signature.
///
/// `primary_subject` is `std::nullopt` when the upstream detector
/// found no bird (`DetectionResult::primary_subject_box` was
/// `nullopt`), in which case the stub still returns the sentinel
/// unchanged. M4-* will short-circuit in that branch.
///
/// \throws FocusError on scoring failure (once M4-* lands). The stub
///         never throws.
[[nodiscard]] FocusResult Score(
    const raw::PreviewImage& preview,
    const std::optional<detect::BBox>& primary_subject,
    const FocusConfig& config);

}  // namespace wildframe::focus
