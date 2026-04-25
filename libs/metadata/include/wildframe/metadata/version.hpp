#pragma once

/// \file
/// Compile-time version constants stamped onto every Wildframe XMP
/// sidecar at write time (TB-07, `docs/METADATA.md` §4).
///
/// `kPipelineVersion` is the semver of the Wildframe binary that
/// produced the sidecar — independent of the detector model version and
/// the focus-algorithm version (both are captured separately at write
/// time). Bump in lockstep with the release tag when a new version
/// ships. Every thickening pass that changes what a sidecar field
/// *means* owns the bump for its own version string; until M3-* / M4-*
/// land, the stub detector / focus algorithm strings are inlined at the
/// writer's definition rather than exposed here (`docs/STYLE.md` §2.11
/// — no speculative public surface).

#include <string_view>

namespace wildframe::metadata {

/// Wildframe binary version stamped onto
/// `wildframe_provenance:pipeline_version`. `"0.0.0"` is the pre-v0.1.0
/// placeholder; it bumps to `"0.1.0"` when the first tagged release
/// ships (handoff §18 Phase 0 exit).
inline constexpr std::string_view kPipelineVersion{"0.0.0"};

}  // namespace wildframe::metadata
