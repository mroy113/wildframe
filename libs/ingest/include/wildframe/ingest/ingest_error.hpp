#pragma once

/// \file
/// `IngestError` — the boundary exception type raised by `wildframe_ingest`
/// public APIs (M1-04, handoff §NFR-7, `docs/STYLE.md` §3.1, §3.2).
///
/// `wildframe_ingest` owns `std::filesystem`. Exceptions from the
/// filesystem library (`std::filesystem::filesystem_error`) are caught
/// at the public-API boundary and translated to `IngestError` so
/// callers never see the third-party exception type. Per-file issues
/// discovered mid-enumeration (unreadable entry, magic-bytes mismatch)
/// are logged and skipped rather than thrown — per `docs/STYLE.md`
/// §3.4, a malformed file in a batch is expected, not exceptional. An
/// `IngestError` therefore signals a problem with the enumeration root
/// itself (missing, not a directory, permission-denied to list).

#include <stdexcept>

namespace wildframe::ingest {

/// Boundary exception raised by `wildframe_ingest` public APIs. Derived
/// from `std::runtime_error` per `docs/STYLE.md` §3.2. Inherits the
/// `const std::string&` and `const char*` constructors of its base —
/// no new state is added, so the compiler-generated special members
/// (Rule of Zero via inheritance) are correct.
class IngestError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

}  // namespace wildframe::ingest
