#pragma once

/// \file
/// `ImageJob` — the value type produced by `wildframe_ingest` and
/// consumed by every downstream pipeline stage (FR-1, handoff §10, §12).
/// One record per discovered RAW file; carries the filesystem location
/// and a typed format tag so later stages do not re-inspect the path.
///
/// The type is a plain data aggregate: public `snake_case` members per
/// `docs/STYLE.md` §2.1, compiler-generated special members (Rule of
/// Zero, STYLE §2.5), copyable and trivially movable. Immutability is a
/// convention — callers construct an `ImageJob` once from an ingest
/// enumeration result and do not mutate it thereafter. `const`-
/// qualifying the members would delete copy-assignment and break the
/// "value type, copyable" contract in `docs/BACKLOG.md` M1-01.

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace wildframe::ingest {

/// RAW container format. MVP supports CR3 only (`CLAUDE.md` §6);
/// additional formats are out of scope until the handoff doc changes.
/// Explicitly `std::uint8_t`-backed — one byte is sufficient for the
/// enumerator set and `performance-enum-size` would otherwise flag the
/// default `int` base.
enum class Format : std::uint8_t {
  kCr3,
};

/// A single RAW file discovered by `wildframe_ingest`. Fields are
/// populated by `enumerate()` (M1-02) after the path has passed CR3
/// validation (M1-03); downstream stages consume this struct by value
/// or `const&`. Default initializers give the aggregate a well-defined
/// zero state (`cppcoreguidelines-pro-type-member-init`) — callers must
/// still populate real values before downstream use.
struct ImageJob {
  /// Absolute canonical path to the RAW file on disk. `enumerate()`
  /// guarantees this is absolute so downstream stages never reinterpret
  /// it against a changing working directory.
  std::filesystem::path path;

  /// Container format tag. MVP always `Format::kCr3`; carried as a
  /// typed field so a Phase 2+ format addition needs no path-string
  /// re-inspection at every stage boundary.
  Format format = Format::kCr3;

  /// File size in bytes, as reported by `std::filesystem::file_size`
  /// at ingest time. Stubbed for M1-01 (`std::nullopt`) and populated
  /// by a later task when a concrete consumer lands (manifest row,
  /// diagnostic log line, or similar). Do not branch on this field in
  /// downstream code until that task lands.
  std::optional<std::uintmax_t> size_bytes;

  /// Content hash of the RAW bytes. Stubbed for M1-01 (`std::nullopt`)
  /// and populated by a later task when a concrete consumer arrives
  /// (re-analysis skip check, dedup, or provenance). Do not branch on
  /// this field in downstream code until that task lands.
  std::optional<std::string> content_hash;
};

}  // namespace wildframe::ingest
