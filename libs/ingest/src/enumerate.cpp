#include "wildframe/ingest/enumerate.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "wildframe/ingest/image_job.hpp"
#include "wildframe/ingest/ingest_error.hpp"
#include "wildframe/log/log.hpp"

namespace wildframe::ingest {

namespace {

/// ISO-BMFF `ftyp` box signature a CR3 file must expose in its first
/// 12 bytes. Bytes 0..3 are the box length (any value); bytes 4..7 are
/// the literal `"ftyp"`; bytes 8..11 are the major brand `"crx "`.
/// See `build/_fixtures/fixture_01_clear_bird.CR3` and Canon CR3 format
/// notes; also matches what LibRaw recognizes. The buffer is `char`
/// throughout to interoperate with `std::istream::read` without a
/// `reinterpret_cast`.
constexpr std::size_t kFtypBoxBytesNeeded = 12;
constexpr std::string_view kFtypTag{"ftyp"};
constexpr std::string_view kCrxBrand{"crx "};

/// Case-insensitive extension check: Canon writes `.CR3`, but a user
/// rename to `.cr3` should still be accepted.
bool HasCr3Extension(const std::filesystem::path& path) {
  std::string ext = path.extension().string();
  std::ranges::transform(ext, ext.begin(), [](unsigned char character) {
    return static_cast<char>(std::tolower(character));
  });
  return ext == ".cr3";
}

/// Read the first 12 bytes of `path` and confirm the ISO-BMFF `ftyp`
/// box identifies a CR3 (brand `crx `). Any I/O failure or short read
/// returns `false` — callers treat it the same as a magic-bytes
/// mismatch (logged and skipped).
bool HasCr3MagicBytes(const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream.is_open()) {
    return false;
  }
  std::array<char, kFtypBoxBytesNeeded> header{};
  stream.read(header.data(), static_cast<std::streamsize>(header.size()));
  if (stream.gcount() != static_cast<std::streamsize>(header.size())) {
    return false;
  }
  const std::string_view view{header.data(), header.size()};
  return view.substr(4, kFtypTag.size()) == kFtypTag &&
         view.substr(8, kCrxBrand.size()) == kCrxBrand;
}

/// Inspect a single directory entry and, on success, append an
/// `ImageJob` to `out`. Per-entry filesystem errors are swallowed with
/// a `warn` log — they are expected (symlink to missing target,
/// permission-denied on one file) and must not abort the batch
/// (FR-1, STYLE §3.4).
void ConsiderEntry(const std::filesystem::directory_entry& entry,
                   std::vector<ImageJob>& out) {
  std::error_code status_error;
  if (entry.is_symlink(status_error) || status_error) {
    if (status_error) {
      log::ingest.warn("skipped entry (status check failed): {} — {}",
                       entry.path().string(), status_error.message());
    }
    return;
  }
  if (!entry.is_regular_file(status_error) || status_error) {
    if (status_error) {
      log::ingest.warn("skipped entry (status check failed): {} — {}",
                       entry.path().string(), status_error.message());
    }
    return;
  }
  if (!HasCr3Extension(entry.path())) {
    return;
  }
  if (!HasCr3MagicBytes(entry.path())) {
    log::ingest.warn("skipped malformed CR3: {} — ftyp/crx magic mismatch",
                     entry.path().string());
    return;
  }

  std::error_code size_error;
  const std::uintmax_t size = entry.file_size(size_error);
  std::error_code canonical_error;
  std::filesystem::path canonical =
      std::filesystem::weakly_canonical(entry.path(), canonical_error);
  if (canonical_error) {
    canonical = entry.path();
  }

  out.push_back(ImageJob{
      .path = std::move(canonical),
      .format = Format::kCr3,
      .size_bytes = size_error ? std::optional<std::uintmax_t>{} : size,
      .content_hash = std::nullopt,
  });
}

}  // namespace

std::vector<ImageJob> Enumerate(const std::filesystem::path& dir,
                                int max_depth) {
  if (max_depth < 0) {
    throw IngestError("ingest: max_depth must be non-negative, got " +
                      std::to_string(max_depth));
  }

  std::vector<ImageJob> jobs;
  try {
    std::error_code stat_error;
    const std::filesystem::file_status status =
        std::filesystem::status(dir, stat_error);
    if (stat_error) {
      throw IngestError("ingest: cannot stat directory " + dir.string() +
                        " — " + stat_error.message());
    }
    if (!std::filesystem::exists(status)) {
      throw IngestError("ingest: directory does not exist: " + dir.string());
    }
    if (!std::filesystem::is_directory(status)) {
      throw IngestError("ingest: not a directory: " + dir.string());
    }

    // `recursive_directory_iterator`'s skip-permission-denied flag
    // keeps the walk going past a forbidden subdirectory (NFR-6: a
    // field batch should not be aborted by one locked folder). We do
    // not follow symlinks — the default behavior.
    constexpr auto kIterOptions =
        std::filesystem::directory_options::skip_permission_denied;

    std::filesystem::recursive_directory_iterator iter(dir, kIterOptions);
    const std::filesystem::recursive_directory_iterator end;
    for (; iter != end; ++iter) {
      // `depth()` returns 0 for entries directly inside `dir`, 1 for
      // their children, etc. `max_depth == 0` therefore inspects only
      // depth-0 entries; disabling recursion at `depth() >= max_depth`
      // prevents descent into every visited directory once we hit the
      // cap.
      if (iter.depth() >= max_depth) {
        iter.disable_recursion_pending();
      }
      ConsiderEntry(*iter, jobs);
    }
  } catch (const std::filesystem::filesystem_error& fs_error) {
    throw IngestError(std::string{"ingest: filesystem error during "
                                  "enumeration of "} +
                      dir.string() + " — " + fs_error.what());
  }

  std::ranges::sort(jobs, [](const ImageJob& lhs, const ImageJob& rhs) {
    return lhs.path < rhs.path;
  });
  return jobs;
}

}  // namespace wildframe::ingest
