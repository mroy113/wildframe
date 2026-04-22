#include "wildframe/orchestrator/paths.hpp"

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace wildframe::orchestrator {

namespace {

#ifdef _WIN32
constexpr const char* kHomeEnvVar = "USERPROFILE";
#else
constexpr const char* kHomeEnvVar = "HOME";
#endif

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
  if (s.empty() || s[0] != '~') {
    return p;
  }
  if (s.size() == 1) {
    return home;
  }
  // Only `~/<rest>` is our contract. `~user/...` stays literal so a
  // user typing someone else's home in their config gets a "path does
  // not exist" later instead of silently resolving to their own home.
  if (s[1] == '/') {
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

std::optional<std::filesystem::path> ResolveHomeFromEnv() {
  // `std::getenv` is flagged `concurrency-mt-unsafe` because it races
  // with `setenv` in another thread. Wildframe's contract is that
  // this is the one seam reading the environment, called once at
  // startup before any worker thread spins up (ARCHITECTURE.md §5).
  // Alternatives (`secure_getenv`, `_NSGetEnviron`) are non-portable
  // and would trade the thread-safety lint for a portability lint.
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  const auto* raw = std::getenv(kHomeEnvVar);
  if (raw == nullptr) {
    return std::nullopt;
  }
  const std::string_view view{raw};
  if (view.empty()) {
    return std::nullopt;
  }
  return std::filesystem::path{view};
}

}  // namespace wildframe::orchestrator
