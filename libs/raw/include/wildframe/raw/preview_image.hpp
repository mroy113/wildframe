#pragma once

/// \file
/// `PreviewImage` — decoded RGB preview the raw module emits and that
/// every downstream pipeline stage consumes (TB-03, FR-2, FR-3, FR-4,
/// handoff §12).
///
/// Plain data aggregate (`docs/STYLE.md` §2.1, §2.5): compiler-
/// generated special members, no invariants. Stages treat it
/// by value or `const&`. The stub implementation landing in TB-03
/// fills `rgb_bytes` with a deterministic gray buffer; M2-01 replaces
/// the stub with a real LibRaw-backed JPEG decode without changing
/// this type.

#include <cstdint>
#include <vector>

namespace wildframe::raw {

/// Packed 8-bit RGB in row-major order. `rgb_bytes.size() == width *
/// height * 3` for a well-formed preview. `width` / `height` use
/// `int` to match the LibRaw API types M2-01 will thread through;
/// narrowing to `std::size_t` at the consumer edge is explicit.
struct PreviewImage {
  int width = 0;
  int height = 0;
  std::vector<std::uint8_t> rgb_bytes;
};

}  // namespace wildframe::raw
