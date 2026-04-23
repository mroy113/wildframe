#include "run.hpp"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <ostream>

#include "cli_args.hpp"
#include "config.hpp"
#include "wildframe/log/log.hpp"

namespace wildframe::cli {

namespace {

// `$HOME` / `$XDG_CONFIG_HOME` are read here and only here, per
// `docs/STYLE.md` §2.12. Every helper below takes resolved paths so
// unit tests drive the parser with synthetic inputs and never mutate
// the process environment.
[[nodiscard]] std::filesystem::path ReadHome() {
  const char* raw = std::getenv("HOME");
  if (raw == nullptr || *raw == '\0') {
    return {};
  }
  return std::filesystem::path{raw};
}

[[nodiscard]] std::optional<std::filesystem::path> ReadXdgConfigHome() {
  const char* raw = std::getenv("XDG_CONFIG_HOME");
  if (raw == nullptr || *raw == '\0') {
    return std::nullopt;
  }
  return std::filesystem::path{raw};
}

[[nodiscard]] wildframe::log::Config LogConfigFrom(const Config& cfg) {
  return wildframe::log::Config{
      .level = cfg.log_level,
      .log_path = cfg.log_path,
  };
}

}  // namespace

int Run(int argc, const char* const* argv, std::ostream& err) {
  ParsedArgs args;
  try {
    args = ParseArgs(argc, argv);
  } catch (const UsageError& e) {
    err << "wildframe: " << e.what() << '\n';
    err << UsageText();
    return kExitUsage;
  }

  if (args.action == Action::kShowHelp) {
    std::cout << UsageText();
    return kExitSuccess;
  }
  if (args.action == Action::kShowVersion) {
    std::cout << VersionText();
    return kExitSuccess;
  }

  Config cfg;
  try {
    const auto home = ReadHome();
    const auto xdg = ReadXdgConfigHome();
    const auto resolved = ResolveConfigPath(args.config_path, home, xdg);
    cfg = LoadConfig(resolved, home);
  } catch (const ConfigError& e) {
    err << "wildframe: " << e.what() << '\n';
    return kExitConfig;
  }

  try {
    wildframe::log::Init(LogConfigFrom(cfg));
  } catch (const std::exception& e) {
    err << "wildframe: log initialization failed: " << e.what() << '\n';
    return kExitRuntime;
  }

  wildframe::log::orchestrator.info("cli started: directory={} manifest_dir={}",
                                    args.directory.string(),
                                    cfg.manifest_dir.string());

  // TB-02 wires the orchestrator here: construct the stage list,
  // hand over `cfg.manifest_dir` and the ingest enumeration of
  // `args.directory`, run the batch synchronously, and translate
  // per-job exceptions into exit codes per FR-5 / M6-04.
  (void)args.directory;

  return kExitSuccess;
}

}  // namespace wildframe::cli
