#include "wildframe/orchestrator/paths.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>

namespace {

namespace wfo = wildframe::orchestrator;

// On POSIX / macOS the seam reads HOME. The tests intentionally
// target the same variable the production code reads so we exercise
// the one path that ships on MVP's supported platform (handoff §3).
// Skipping Windows (USERPROFILE) branches keeps the harness from
// speculatively guarding against a platform P2-01 has not decided on.
constexpr const char* kHomeEnvVar = "HOME";

// RAII guard around HOME so no test leaves the process environment
// mutated for subsequent tests or the surrounding ctest runner.
class ScopedHome {
 public:
  ScopedHome() {
    if (const auto* raw = std::getenv(kHomeEnvVar); raw != nullptr) {
      saved_ = std::string{raw};
      had_saved_ = true;
    }
  }

  ScopedHome(const ScopedHome&) = delete;
  ScopedHome& operator=(const ScopedHome&) = delete;
  ScopedHome(ScopedHome&&) = delete;
  ScopedHome& operator=(ScopedHome&&) = delete;

  ~ScopedHome() {
    if (had_saved_) {
      ::setenv(kHomeEnvVar, saved_.c_str(), /*overwrite=*/1);
    } else {
      ::unsetenv(kHomeEnvVar);
    }
  }

  static void Set(const std::string& value) {
    ::setenv(kHomeEnvVar, value.c_str(), /*overwrite=*/1);
  }

  static void Unset() { ::unsetenv(kHomeEnvVar); }

 private:
  std::string saved_{};
  bool had_saved_ = false;
};

TEST(ExpandTilde, EmptyPathReturnedUnchanged) {
  const std::filesystem::path home{"/Users/alice"};
  EXPECT_EQ(wfo::ExpandTilde(std::filesystem::path{}, home),
            std::filesystem::path{});
}

TEST(ExpandTilde, AbsolutePathReturnedUnchanged) {
  const std::filesystem::path home{"/Users/alice"};
  const std::filesystem::path in{"/var/log/foo.log"};
  EXPECT_EQ(wfo::ExpandTilde(in, home), in);
}

TEST(ExpandTilde, RelativePathReturnedUnchanged) {
  const std::filesystem::path home{"/Users/alice"};
  const std::filesystem::path in{"relative/path.json"};
  EXPECT_EQ(wfo::ExpandTilde(in, home), in);
}

TEST(ExpandTilde, BareTildeExpandsToHome) {
  const std::filesystem::path home{"/Users/alice"};
  EXPECT_EQ(wfo::ExpandTilde(std::filesystem::path{"~"}, home), home);
}

TEST(ExpandTilde, TildeSlashExpandsToHomeWithoutTrailingSlash) {
  const std::filesystem::path home{"/Users/alice"};
  // `home / ""` canonicalizes to `home` — not `home/`. Assert on the
  // concrete output rather than any assumption about trailing slash.
  EXPECT_EQ(wfo::ExpandTilde(std::filesystem::path{"~/"}, home), home);
}

TEST(ExpandTilde, TildeSlashRestExpandsToHomeSlashRest) {
  const std::filesystem::path home{"/Users/alice"};
  EXPECT_EQ(wfo::ExpandTilde(
                std::filesystem::path{"~/Library/Logs/wildframe.log"}, home),
            std::filesystem::path{"/Users/alice/Library/Logs/wildframe.log"});
}

TEST(ExpandTilde, TildeUserNotExpanded) {
  const std::filesystem::path home{"/Users/alice"};
  // Per docs/CONFIG.md §1.3 we do not resolve other users' home dirs.
  const std::filesystem::path in{"~bob/Documents"};
  EXPECT_EQ(wfo::ExpandTilde(in, home), in);
}

TEST(ExpandTilde, InteriorTildeNotExpanded) {
  const std::filesystem::path home{"/Users/alice"};
  // Only leading `~` is magical — `foo~bar` is a literal filename.
  const std::filesystem::path in{"/var/log/foo~bar"};
  EXPECT_EQ(wfo::ExpandTilde(in, home), in);
}

TEST(DefaultLogPath, ResolvesAgainstHome) {
  const std::filesystem::path home{"/Users/alice"};
  EXPECT_EQ(wfo::DefaultLogPath(home),
            std::filesystem::path{
                "/Users/alice/Library/Logs/Wildframe/wildframe.log"});
}

TEST(DefaultManifestDir, ResolvesAgainstHome) {
  const std::filesystem::path home{"/Users/alice"};
  EXPECT_EQ(wfo::DefaultManifestDir(home),
            std::filesystem::path{
                "/Users/alice/Library/Application Support/Wildframe/batches"});
}

TEST(ResolveHomeFromEnv, ReturnsPathWhenHomeSet) {
  const ScopedHome guard;
  ScopedHome::Set("/Users/alice");

  const auto resolved = wfo::ResolveHomeFromEnv();
  ASSERT_TRUE(resolved.has_value());
  EXPECT_EQ(*resolved, std::filesystem::path{"/Users/alice"});
}

TEST(ResolveHomeFromEnv, ReturnsNulloptWhenHomeUnset) {
  const ScopedHome guard;
  ScopedHome::Unset();

  EXPECT_EQ(wfo::ResolveHomeFromEnv(), std::nullopt);
}

TEST(ResolveHomeFromEnv, ReturnsNulloptWhenHomeEmpty) {
  const ScopedHome guard;
  ScopedHome::Set("");

  EXPECT_EQ(wfo::ResolveHomeFromEnv(), std::nullopt);
}

}  // namespace
