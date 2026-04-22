#include "wildframe/orchestrator/paths.hpp"

#include <filesystem>
#include <string>

namespace wildframe::orchestrator {

namespace {

// macOS user-data layout per `docs/CONFIG.md` §3.1. The relative
// portions are spelled out here so the resolved absolutes appear
// verbatim in the tests; a split across multiple `operator/` calls
// would also work but obscures the diff when §3.1 moves.
constexpr const char* kLogPathRelative = "Library/Logs/Wildframe/wildframe.log";
constexpr const char* kManifestDirRelative =
    "Library/Application Support/Wildframe/batches";

}  // namespace

std::filesystem::path ExpandTilde(const std::filesystem::path& p,
                                  const std::filesystem::path& home) {
  const std::string s = p.string();
  if (!s.starts_with('~')) {
    return p;
  }
  if (s.size() == 1) {
    return home;
  }
  // Only `~/<rest>` is our contract. `~user/...` stays literal so a
  // user typing someone else's home in their config gets a "path does
  // not exist" later instead of silently resolving to their own home.
  if (s.starts_with("~/")) {
    const auto rest = s.substr(2);
    // `home / ""` appends a separator in libc++, so treat empty tail
    // (`~/`) the same as bare `~`.
    return rest.empty() ? home : home / rest;
  }
  return p;
}

std::filesystem::path DefaultLogPath(const std::filesystem::path& home) {
  return home / kLogPathRelative;
}

std::filesystem::path DefaultManifestDir(const std::filesystem::path& home) {
  return home / kManifestDirRelative;
}

}  // namespace wildframe::orchestrator
