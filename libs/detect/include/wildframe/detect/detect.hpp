#pragma once

/// \file
/// Public API for `wildframe_detect` (Module 3, FR-3, handoff §10).
///
/// The detector consumes a decoded preview and returns the bird-class
/// detections that pass confidence + NMS. TB-04 introduces the public
/// types and the stub entry point that returns a deterministic
/// "no birds" sentinel; M3-01..M3-05 wire the real YOLOv11 path
/// behind the same signatures without changing this header.
///
/// Exception policy (`docs/STYLE.md` §3): the real implementation will
/// translate `Ort::Exception` at this boundary to
/// `wildframe::detect::DetectError`. The TB-04 stub never throws.

#include <cstdint>
#include <optional>
#include <vector>

#include "wildframe/detect/bbox.hpp"
#include "wildframe/raw/preview_image.hpp"

namespace wildframe::detect {

/// Runtime-tunable detector knobs. Empty in TB-04 — real fields
/// (`confidence_threshold`, `iou_threshold`, `model`,
/// `execution_provider`) land with M3-01 / M3-03 / M3-06 in the same
/// PR that widens TB-01's TOML parser with the matching `[detect]`
/// keys. Declared now so the stub's signature is stable across the
/// thickening pass (`docs/STYLE.md` §2.11 — the sole caller is the
/// stub stage landing in this same task).
struct DetectConfig {};

/// Per-image detection output. Field shape mirrors the AI namespace
/// in `docs/METADATA.md` §3 except that the in-memory primary subject
/// travels as an `std::optional<BBox>` rather than the on-disk
/// `primary_subject_index` — the index-into-`bird_boxes` encoding is
/// a serialization detail M5-08 handles, while downstream in-memory
/// consumers (`wildframe_focus::Score`, FR-4) just need the box.
///
/// Stub returns every field at its "no detection" value:
/// `bird_present=false`, `bird_count=0`, `bird_boxes={}`,
/// `primary_subject_box=std::nullopt`, `detection_confidence=0.0F`.
/// Consumers that treat `bird_present=false` as the signal to skip
/// focus scoring and emit `keeper_score=0.0` (per `docs/METADATA.md`
/// §3) therefore see the correct shape unchanged when M3-* lands.
///
/// Plain aggregate (`docs/STYLE.md` §2.1): bare `snake_case` fields,
/// Rule of Zero.
struct DetectionResult {
  bool bird_present = false;
  std::int32_t bird_count = 0;
  std::vector<BBox> bird_boxes;
  std::optional<BBox> primary_subject_box;
  float detection_confidence = 0.0F;
};

/// Run the detector against one decoded preview. Stub returns the
/// "no detection" sentinel described on `DetectionResult`; M3-05
/// replaces the body with a real ONNX Runtime call under the same
/// signature.
///
/// \throws DetectError on inference failure (once M3-* lands). The
///         stub never throws.
[[nodiscard]] DetectionResult Detect(const raw::PreviewImage& preview,
                                     const DetectConfig& config);

}  // namespace wildframe::detect
