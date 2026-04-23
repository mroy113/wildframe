#include "cli_args.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <string_view>

namespace {

namespace cli = wildframe::cli;

// Small wrapper so tests can spell invocations as brace-init lists
// without reinterpret-casting through `char* const*`.
template <std::size_t N>
cli::ParsedArgs Parse(const std::array<const char*, N>& argv) {
  return cli::ParseArgs(static_cast<int>(argv.size()), argv.data());
}

TEST(CliArgsParse, DirectoryPositionalOnly) {
  const std::array<const char*, 2> argv{"wildframe", "/tmp/raws"};
  const auto parsed = Parse(argv);
  EXPECT_EQ(parsed.action, cli::Action::kRun);
  EXPECT_FALSE(parsed.config_path.has_value());
  EXPECT_EQ(parsed.directory, std::filesystem::path{"/tmp/raws"});
}

TEST(CliArgsParse, ConfigAndDirectory) {
  const std::array<const char*, 4> argv{"wildframe", "--config", "/etc/wf.toml",
                                        "/tmp/raws"};
  const auto parsed = Parse(argv);
  EXPECT_EQ(parsed.action, cli::Action::kRun);
  ASSERT_TRUE(parsed.config_path.has_value());
  if (parsed.config_path.has_value()) {
    EXPECT_EQ(*parsed.config_path, std::filesystem::path{"/etc/wf.toml"});
  }
  EXPECT_EQ(parsed.directory, std::filesystem::path{"/tmp/raws"});
}

TEST(CliArgsParse, DirectoryBeforeConfigAlsoAccepted) {
  // Argv order is flexible; positional can precede or follow flags.
  const std::array<const char*, 4> argv{"wildframe", "/tmp/raws", "--config",
                                        "/etc/wf.toml"};
  const auto parsed = Parse(argv);
  EXPECT_EQ(parsed.directory, std::filesystem::path{"/tmp/raws"});
  ASSERT_TRUE(parsed.config_path.has_value());
  if (parsed.config_path.has_value()) {
    EXPECT_EQ(*parsed.config_path, std::filesystem::path{"/etc/wf.toml"});
  }
}

TEST(CliArgsParse, HelpLongFlag) {
  const std::array<const char*, 2> argv{"wildframe", "--help"};
  EXPECT_EQ(Parse(argv).action, cli::Action::kShowHelp);
}

TEST(CliArgsParse, HelpShortFlag) {
  const std::array<const char*, 2> argv{"wildframe", "-h"};
  EXPECT_EQ(Parse(argv).action, cli::Action::kShowHelp);
}

TEST(CliArgsParse, VersionLongFlag) {
  const std::array<const char*, 2> argv{"wildframe", "--version"};
  EXPECT_EQ(Parse(argv).action, cli::Action::kShowVersion);
}

TEST(CliArgsParse, VersionShortFlag) {
  const std::array<const char*, 2> argv{"wildframe", "-v"};
  EXPECT_EQ(Parse(argv).action, cli::Action::kShowVersion);
}

TEST(CliArgsParse, HelpWinsOverPositional) {
  // --help short-circuits before the missing-directory check fires,
  // so `wildframe --help` is a valid invocation.
  const std::array<const char*, 2> argv{"wildframe", "--help"};
  EXPECT_NO_THROW((void)Parse(argv));
}

TEST(CliArgsParse, MissingDirectoryThrows) {
  const std::array<const char*, 1> argv{"wildframe"};
  EXPECT_THROW((void)Parse(argv), cli::UsageError);
}

TEST(CliArgsParse, ConfigWithoutValueThrows) {
  const std::array<const char*, 2> argv{"wildframe", "--config"};
  EXPECT_THROW((void)Parse(argv), cli::UsageError);
}

TEST(CliArgsParse, UnknownFlagThrows) {
  const std::array<const char*, 3> argv{"wildframe", "--bogus", "/tmp/raws"};
  EXPECT_THROW((void)Parse(argv), cli::UsageError);
}

TEST(CliArgsParse, ExtraPositionalThrows) {
  const std::array<const char*, 4> argv{"wildframe", "/tmp/raws", "/tmp/other",
                                        "--config"};
  EXPECT_THROW((void)Parse(argv), cli::UsageError);
}

TEST(CliArgsParse, UsageTextMentionsExitCodes) {
  const auto text = cli::UsageText();
  EXPECT_NE(text.find("Exit codes"), std::string_view::npos);
  EXPECT_NE(text.find("--config"), std::string_view::npos);
}

TEST(CliArgsParse, VersionTextStartsWithProgramName) {
  EXPECT_TRUE(cli::VersionText().starts_with("wildframe "));
}

}  // namespace
