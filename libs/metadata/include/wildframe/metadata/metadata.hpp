#pragma once

/// \file
/// Public API for `wildframe_metadata` (Module 5, FR-5, handoff §10).
///
/// The metadata module is the sole owner of Exiv2 and covers three
/// concerns per handoff §10: deterministic EXIF read from the RAW file,
/// XMP sidecar write (AI + provenance), and XMP sidecar read for the
/// review UI. TB-06 introduces the public `ReadExif` entry point and
/// the stub that returns the filesystem-derived fields only, every
/// EXIF-sourced field `std::nullopt`; M5-02..M5-04 wire the real Exiv2
/// reader behind the same signature without changing this header.
///
/// Exception policy (`docs/STYLE.md` §3): the real implementation will
/// translate exceptions raised by Exiv2 at this boundary to
/// `wildframe::metadata::MetadataError`. The TB-06 stub never throws.

#include <filesystem>

#include "wildframe/metadata/deterministic_metadata.hpp"

namespace wildframe::metadata {

/// Read the deterministic (camera-recorded) metadata block from a RAW
/// file's EXIF tags. Stub returns a `DeterministicMetadata` with the
/// filesystem-derived fields populated from `raw_path` and every
/// EXIF-sourced field as `std::nullopt`; M5-02 replaces the body with a
/// real Exiv2 read under the same signature.
///
/// `raw_path` is passed through into the returned struct unchanged — it
/// is the caller's responsibility to hand in an absolute, canonical path
/// (the ingest stage's `ImageJob::path` guarantee per FR-1).
///
/// \throws MetadataError on Exiv2 failure (once M5-03 lands). The stub
///         never throws.
[[nodiscard]] DeterministicMetadata ReadExif(
    const std::filesystem::path& raw_path);

}  // namespace wildframe::metadata
