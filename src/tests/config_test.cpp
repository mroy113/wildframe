#include "config.hpp"

#include <gtest/gtest.h>
#include <spdlog/common.h>

#include <filesystem>
#include <fstream>
#include <ios>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace {

namespace cli = wildframe::cli;

class ConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
    tmp_ = std::filesystem::path{testing::TempDir()} /
           ::testing::UnitTest::GetInstance()->current_test_info()->name();
    std::filesystem::remove_all(tmp_);
    std::filesystem::create_directories(tmp_);

    // A synthetic, never-mutated stand-in for $HOME. Tests never
    // touch the real environment per docs/STYLE.md §2.12.
    home_ = tmp_ / "home";
    std::filesystem::create_directories(home_);
  }

  void TearDown() override {
    std::error_code ignored;
    std::filesystem::remove_all(tmp_, ignored);
  }

  std::filesystem::path WriteToml(std::string_view contents) const {
    const auto path = tmp_ / "config.toml";
    std::ofstream stream(path, std::ios::binary);
    stream.write(contents.data(),
                 static_cast<std::streamsize>(contents.size()));
    return path;
  }

  std::filesystem::path tmp_;
  std::filesystem::path home_;
};

// --- BuiltInDefaults -------------------------------------------------------

TEST_F(ConfigTest, BuiltInDefaultsResolveAgainstHome) {
  const auto cfg = cli::BuiltInDefaults(home_);
  EXPECT_EQ(cfg.log_path,
            home_ / "Library" / "Logs" / "Wildframe" / "wildframe.log");
  EXPECT_EQ(cfg.manifest_dir, home_ / "Library" / "Application Support" /
                                  "Wildframe" / "batches");
  EXPECT_EQ(cfg.reanalysis_default, cli::ReanalysisPolicy::kPrompt);
}

TEST_F(ConfigTest, BuiltInDefaultsEmptyHomeThrows) {
  EXPECT_THROW((void)cli::BuiltInDefaults(std::filesystem::path{}),
               cli::ConfigError);
}

// --- LoadConfig: no user file ---------------------------------------------

TEST_F(ConfigTest, LoadConfigWithoutUserFileReturnsDefaults) {
  const auto cfg = cli::LoadConfig(std::nullopt, home_);
  const auto defaults = cli::BuiltInDefaults(home_);
  EXPECT_EQ(cfg.log_path, defaults.log_path);
  EXPECT_EQ(cfg.manifest_dir, defaults.manifest_dir);
  EXPECT_EQ(cfg.log_level, defaults.log_level);
  EXPECT_EQ(cfg.reanalysis_default, defaults.reanalysis_default);
}

// --- LoadConfig: key overrides --------------------------------------------

TEST_F(ConfigTest, LoadConfigLogPathOverride) {
  const auto file = WriteToml("log_path = \"/var/log/wf.log\"\n");
  const auto cfg = cli::LoadConfig(file, home_);
  EXPECT_EQ(cfg.log_path, std::filesystem::path{"/var/log/wf.log"});
}

TEST_F(ConfigTest, LoadConfigLogPathTildeExpands) {
  const auto file = WriteToml("log_path = \"~/logs/custom.log\"\n");
  const auto cfg = cli::LoadConfig(file, home_);
  EXPECT_EQ(cfg.log_path, home_ / "logs" / "custom.log");
}

TEST_F(ConfigTest, LoadConfigTildeWithoutHomeThrows) {
  const auto file = WriteToml("log_path = \"~/logs/custom.log\"\n");
  EXPECT_THROW((void)cli::LoadConfig(file, std::filesystem::path{}),
               cli::ConfigError);
}

TEST_F(ConfigTest, LoadConfigManifestDirOverride) {
  const auto file = WriteToml("manifest_dir = \"/srv/wf/manifests\"\n");
  const auto cfg = cli::LoadConfig(file, home_);
  EXPECT_EQ(cfg.manifest_dir, std::filesystem::path{"/srv/wf/manifests"});
}

TEST_F(ConfigTest, LoadConfigLogLevelOverride) {
  // spdlog's canonical name for the warn level is "warning"
  // (SPDLOG_LEVEL_NAMES in spdlog/common.h). docs/CONFIG.md §3.1
  // lists the spdlog spellings verbatim.
  const auto file = WriteToml("log_level = \"warning\"\n");
  const auto cfg = cli::LoadConfig(file, home_);
  EXPECT_EQ(cfg.log_level, spdlog::level::warn);
}

TEST_F(ConfigTest, LoadConfigLogLevelErrorMapsToSpdlogErr) {
  const auto file = WriteToml("log_level = \"error\"\n");
  const auto cfg = cli::LoadConfig(file, home_);
  EXPECT_EQ(cfg.log_level, spdlog::level::err);
}

TEST_F(ConfigTest, LoadConfigLogLevelOffRejected) {
  // spdlog accepts "off" but Wildframe does not — silencing all logs
  // would defeat the NFR-5 audit trail, and it is not in
  // docs/CONFIG.md §3.1's allowed list.
  const auto file = WriteToml("log_level = \"off\"\n");
  EXPECT_THROW((void)cli::LoadConfig(file, home_), cli::ConfigError);
}

TEST_F(ConfigTest, LoadConfigReanalysisDefaultOverride) {
  const auto file = WriteToml("reanalysis_default = \"overwrite\"\n");
  const auto cfg = cli::LoadConfig(file, home_);
  EXPECT_EQ(cfg.reanalysis_default, cli::ReanalysisPolicy::kOverwrite);
}

TEST_F(ConfigTest, LoadConfigPartialFileKeepsOtherDefaults) {
  const auto file = WriteToml("log_level = \"warn\"\n");
  const auto cfg = cli::LoadConfig(file, home_);
  const auto defaults = cli::BuiltInDefaults(home_);
  EXPECT_EQ(cfg.log_level, spdlog::level::warn);
  EXPECT_EQ(cfg.log_path, defaults.log_path);
  EXPECT_EQ(cfg.manifest_dir, defaults.manifest_dir);
  EXPECT_EQ(cfg.reanalysis_default, defaults.reanalysis_default);
}

// --- LoadConfig: rejections ------------------------------------------------

TEST_F(ConfigTest, LoadConfigUnknownTopLevelKeyThrows) {
  const auto file = WriteToml("mystery_key = 42\n");
  EXPECT_THROW((void)cli::LoadConfig(file, home_), cli::ConfigError);
}

TEST_F(ConfigTest, LoadConfigDetectTableRejectedInTb01) {
  // Per docs/BACKLOG.md TB-01: "An unknown-key rejection fires on the
  // [detect] table today — that is intentional for the slice." M3-*
  // widens the parser in the PR that adds the real detect consumer.
  const auto file = WriteToml(
      "[detect]\n"
      "model = \"yolov11\"\n");
  EXPECT_THROW((void)cli::LoadConfig(file, home_), cli::ConfigError);
}

TEST_F(ConfigTest, LoadConfigWrongTypeThrows) {
  const auto file = WriteToml("log_path = 42\n");
  EXPECT_THROW((void)cli::LoadConfig(file, home_), cli::ConfigError);
}

TEST_F(ConfigTest, LoadConfigInvalidLogLevelThrows) {
  const auto file = WriteToml("log_level = \"verbose\"\n");
  EXPECT_THROW((void)cli::LoadConfig(file, home_), cli::ConfigError);
}

TEST_F(ConfigTest, LoadConfigInvalidReanalysisDefaultThrows) {
  const auto file = WriteToml("reanalysis_default = \"maybe\"\n");
  EXPECT_THROW((void)cli::LoadConfig(file, home_), cli::ConfigError);
}

TEST_F(ConfigTest, LoadConfigTomlSyntaxErrorThrows) {
  const auto file = WriteToml("log_path = \n");
  EXPECT_THROW((void)cli::LoadConfig(file, home_), cli::ConfigError);
}

// --- ResolveConfigPath ----------------------------------------------------

TEST_F(ConfigTest, ResolveConfigPathCliOverrideExistingWins) {
  const auto file = tmp_ / "explicit.toml";
  std::ofstream(file) << "";  // NOLINT(bugprone-unused-raii) — write empty.
  const auto resolved =
      cli::ResolveConfigPath(file, home_, /*xdg_config_home=*/std::nullopt);
  ASSERT_TRUE(resolved.has_value());
  if (resolved.has_value()) {
    EXPECT_EQ(*resolved, file);
  }
}

TEST_F(ConfigTest, ResolveConfigPathCliOverrideMissingThrows) {
  const auto missing = tmp_ / "does-not-exist.toml";
  EXPECT_THROW((void)cli::ResolveConfigPath(missing, home_, std::nullopt),
               cli::ConfigError);
}

TEST_F(ConfigTest, ResolveConfigPathCliOverrideTildeExpands) {
  std::filesystem::create_directories(home_ / "cfg");
  const auto file = home_ / "cfg" / "wf.toml";
  std::ofstream(file) << "";  // NOLINT(bugprone-unused-raii)
  const auto resolved = cli::ResolveConfigPath(
      std::filesystem::path{"~/cfg/wf.toml"}, home_, std::nullopt);
  ASSERT_TRUE(resolved.has_value());
  if (resolved.has_value()) {
    EXPECT_EQ(*resolved, file);
  }
}

TEST_F(ConfigTest, ResolveConfigPathXdgConfigHomeHit) {
  const auto xdg = tmp_ / "xdg";
  std::filesystem::create_directories(xdg / "wildframe");
  const auto expected = xdg / "wildframe" / "config.toml";
  std::ofstream(expected) << "";  // NOLINT(bugprone-unused-raii)

  const auto resolved =
      cli::ResolveConfigPath(std::nullopt, home_, std::optional{xdg});
  ASSERT_TRUE(resolved.has_value());
  if (resolved.has_value()) {
    EXPECT_EQ(*resolved, expected);
  }
}

TEST_F(ConfigTest, ResolveConfigPathMacFallbackHit) {
  const auto dir = home_ / "Library" / "Application Support" / "Wildframe";
  std::filesystem::create_directories(dir);
  const auto expected = dir / "config.toml";
  std::ofstream(expected) << "";  // NOLINT(bugprone-unused-raii)

  const auto resolved =
      cli::ResolveConfigPath(std::nullopt, home_, std::nullopt);
  ASSERT_TRUE(resolved.has_value());
  if (resolved.has_value()) {
    EXPECT_EQ(*resolved, expected);
  }
}

TEST_F(ConfigTest, ResolveConfigPathNoMatchReturnsNullopt) {
  const auto resolved =
      cli::ResolveConfigPath(std::nullopt, home_, std::nullopt);
  EXPECT_FALSE(resolved.has_value());
}

}  // namespace
