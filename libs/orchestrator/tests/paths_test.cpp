#include "wildframe/orchestrator/paths.hpp"

#include <gtest/gtest.h>

#include <filesystem>

namespace {

namespace wfo = wildframe::orchestrator;

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

}  // namespace
