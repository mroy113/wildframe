#pragma once

/// \file
/// `WriteProvenanceSidecar` — provenance-only XMP sidecar writer
/// (TB-07, handoff §13, `docs/METADATA.md` §4, FR-5, NFR-5).
///
/// Writes the five Sprint 2 `wildframe_provenance:*` fields to
/// `<raw_path>.xmp` alongside the RAW file. The `wildframe:*` AI
/// namespace and `wildframe_user:*` override namespace are deliberately
/// deferred until M5-05 / M5-08 / M5-10 — writing sentinel AI values
/// during the tracer-bullet slice would pollute the round-trip tests
/// those thickening passes add.
///
/// The writer is atomic: it serializes the XMP packet, writes it to a
/// temporary file in the same directory as the sidecar, and renames
/// the temporary over the target on success. A crash or exception
/// during the write leaves any pre-existing sidecar untouched
/// (`docs/METADATA.md` §1.2). The read-modify-write path preserves
/// every XMP node outside the `wildframe_provenance:*` namespace, so
/// Lightroom / Bridge / exiftool writes on the same sidecar survive a
/// Wildframe re-analysis (`docs/METADATA.md` §1.3).
///
/// M5-08 reuses this machinery unchanged to layer the `wildframe:*` AI
/// namespace on top — the atomicity and preservation guarantees are
/// the same contract.

#include <chrono>
#include <filesystem>

namespace wildframe::metadata {

/// Write (or update) the provenance-only XMP sidecar next to
/// `raw_path`. The sidecar path is `raw_path + ".xmp"` per
/// `docs/METADATA.md` §1.1 — the RAW extension is kept so the sidecar
/// sits alongside tool-native writes (Adobe Bridge, Lightroom Classic,
/// exiftool all use the same convention).
///
/// `analysis_timestamp` is formatted as ISO-8601 UTC with the `Z`
/// suffix per `docs/METADATA.md` §7.2. Passed in by the caller rather
/// than resolved from a process clock inside the writer, per
/// `docs/STYLE.md` §2.12 — the orchestrator captures one instant per
/// run so every sidecar in a batch reports the same timestamp
/// (`docs/METADATA.md` §7.2).
///
/// `raw_path` must refer to an existing RAW file in a writable
/// directory; the sidecar is placed in that directory.
///
/// \throws MetadataError on Exiv2 encode/decode failure, inability to
///         open the temporary file, or atomic-rename failure. The
///         target sidecar (if it pre-existed) is left untouched on
///         any thrown exception.
void WriteProvenanceSidecar(
    const std::filesystem::path& raw_path,
    std::chrono::system_clock::time_point analysis_timestamp);

}  // namespace wildframe::metadata
