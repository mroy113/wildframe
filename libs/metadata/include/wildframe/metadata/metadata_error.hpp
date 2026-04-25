#pragma once

/// \file
/// `MetadataError` — module-boundary exception type for
/// `wildframe_metadata` (TB-07, `docs/STYLE.md` §3.1, §3.2).
///
/// Every public entry point in `wildframe_metadata` that reaches Exiv2
/// catches `Exiv2::Error` (and related third-party types) internally
/// and rethrows a `MetadataError` so callers never have to link Exiv2
/// or catch its exception types. Matches the one-error-per-module
/// pattern documented in `docs/ARCHITECTURE.md` §1 "Third-party
/// dependency boundaries".

#include <stdexcept>

namespace wildframe::metadata {

/// Raised by `wildframe_metadata` public APIs on any XMP / sidecar
/// failure — Exiv2 encode/decode errors, filesystem I/O failures, and
/// the module's own invariant checks. Derives from `std::runtime_error`
/// so callers can log `what()` without downcasting.
class MetadataError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

}  // namespace wildframe::metadata
