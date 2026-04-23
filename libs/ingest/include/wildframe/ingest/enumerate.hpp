#pragma once

/// \file
/// `Enumerate` — directory enumeration entry point for `wildframe_ingest`
/// (M1-02, M1-03, FR-1, handoff §10, §12).
///
/// Walks a user-selected directory and returns a deterministic,
/// path-sorted vector of `ImageJob` records: one per file that passes
/// both the CR3 extension check and the CR3 `ftyp` magic-bytes check
/// (M1-03). Files that fail either check are logged at `warn` and
/// skipped. Symlinks are never followed or emitted in MVP — the
/// thread-safety contract below assumes a stable directory tree and
/// symlink cycles would violate it.
///
/// Thread-safety: this function is safe to call concurrently on
/// disjoint directories; callers must serialize calls that target the
/// same directory subtree if another thread may be mutating the tree.
///
/// Exception policy (`docs/STYLE.md` §3.1, M1-04): any
/// `std::filesystem::filesystem_error` raised while resolving the
/// root or iterating the tree is caught and re-thrown as
/// `wildframe::ingest::IngestError`. Per-file errors discovered
/// mid-iteration are not exceptional (a malformed CR3 in a batch of
/// 500 is expected, STYLE §3.4) — they are logged and the entry is
/// skipped.

#include <filesystem>
#include <vector>

#include "wildframe/ingest/image_job.hpp"

namespace wildframe::ingest {

/// Enumerate CR3 files under `dir` to a depth of `max_depth`
/// directories. `max_depth == 0` inspects only `dir` itself; the
/// default of `1` matches the [ingest].max_depth default documented
/// in [docs/CONFIG.md] and keeps the batch surface predictable for
/// interactive use.
///
/// \param dir       Root directory. Must exist and be a directory;
///                  relative paths are resolved against the caller's
///                  working directory before enumeration.
/// \param max_depth Non-negative maximum recursion depth. Negative
///                  values are rejected.
///
/// \returns         `ImageJob` records sorted by absolute path for
///                  determinism. Empty if no CR3 files qualify.
///
/// \throws IngestError if `dir` does not exist, is not a directory,
///                     cannot be iterated, or `max_depth` is
///                     negative.
[[nodiscard]] std::vector<ImageJob> Enumerate(const std::filesystem::path& dir,
                                              int max_depth = 1);

}  // namespace wildframe::ingest
