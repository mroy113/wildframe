#include "run.hpp"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <utility>
#include <vector>

#include "cli_args.hpp"
#include "config.hpp"
#include "wildframe/detect/detect_stage.hpp"
#include "wildframe/ingest/enumerate.hpp"
#include "wildframe/ingest/image_job.hpp"
#include "wildframe/ingest/ingest_error.hpp"
#include "wildframe/log/log.hpp"
#include "wildframe/orchestrator/orchestrator.hpp"
#include "wildframe/orchestrator/pipeline_stage.hpp"
#include "wildframe/raw/raw_stage.hpp"

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

  std::vector<wildframe::ingest::ImageJob> jobs;
  try {
    jobs = wildframe::ingest::Enumerate(args.directory);
  } catch (const wildframe::ingest::IngestError& ingest_error) {
    err << "wildframe: " << ingest_error.what() << '\n';
    return kExitRuntime;
  }

  // Stage list grows one entry per tracer-bullet stub as
  // TB-03..TB-07 land. TB-03 adds the raw preview stage; TB-04 adds
  // the detect stub; subsequent tasks append their own stages in the
  // same pattern.
  std::vector<std::unique_ptr<wildframe::orchestrator::PipelineStage>> stages;
  stages.push_back(wildframe::raw::MakeRawStage());
  stages.push_back(wildframe::detect::MakeDetectStage());
  wildframe::orchestrator::Orchestrator orchestrator(std::move(stages),
                                                     cfg.manifest_dir);

  try {
    (void)orchestrator.Run(jobs);
  } catch (const std::exception& run_error) {
    // Per-image error isolation is M6-04; until then, a stage
    // exception aborts the batch and the CLI surfaces it as a
    // runtime failure.
    err << "wildframe: " << run_error.what() << '\n';
    return kExitRuntime;
  }

  return kExitSuccess;
}

}  // namespace wildframe::cli
