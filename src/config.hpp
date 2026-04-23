#pragma once

/// \file
/// TOML runtime-config loader for the Wildframe CLI (TB-01, FR-11).
///
/// Lives in `src/` alongside `main.cpp` because config parsing is a
/// GUI/CLI concern, not a module concern (`docs/ARCHITECTURE.md` §6).
/// Every parsing entry point takes resolved `std::filesystem::path`
/// values — environment reads (`$HOME`, `$XDG_CONFIG_HOME`) happen
/// once at the CLI boundary per `docs/STYLE.md` §2.12 so unit tests
/// drive the parser with synthetic paths and never mutate process
/// state.
///
/// Scope for TB-01: parses only the four top-level keys with live
/// consumers in Sprint 2 (`log_path`, `log_level`, `manifest_dir`,
/// `reanalysis_default`). `[ingest]`, `[detect]`, `[focus]` tables
/// are not recognized and trigger the §1.5 unknown-key rejection —
/// each module's thickening pass widens the parser when it adds the
/// first real consumer.

#include <spdlog/common.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>

namespace wildframe::cli {

/// Runtime choice for the `reanalysis_default` TOML key (FR-10). Only
/// validated at parse time by TB-01; the GUI prompt in M7-10 is the
/// first actual consumer.
enum class ReanalysisPolicy : std::uint8_t {
  kPrompt,
  kSkip,
  kOverwrite,
};

/// Strongly-typed, pre-validated config. Downstream code (`Run()`,
/// `wildframe::log::Init()`, eventually the orchestrator) receives
/// this struct and does no further validation per `docs/CONFIG.md`
/// §1.6.
struct Config {
  std::filesystem::path log_path;
  spdlog::level::level_enum log_level = spdlog::level::info;
  std::filesystem::path manifest_dir;
  ReanalysisPolicy reanalysis_default = ReanalysisPolicy::kPrompt;
};

/// Thrown by the loader on any `docs/CONFIG.md` §5 failure:
/// nonexistent `--config` path, TOML syntax error, unknown key,
/// wrong type, out-of-range value, or `~` without a resolvable
/// `$HOME`. `Run()` maps this to exit code `kExitConfig`.
class ConfigError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/// Resolve the config path per `docs/CONFIG.md` §1.2. Returns
/// `nullopt` when no user config file is found — callers then use
/// the built-in defaults.
///
/// Order:
///   1. `cli_override` if set — must exist (otherwise throws).
///   2. `<xdg_config_home>/wildframe/config.toml` if `xdg_config_home`
///      is set and the file exists.
///   3. `<home>/Library/Application Support/Wildframe/config.toml`
///      if it exists (macOS fallback).
///   4. `nullopt` — no user file; defaults win.
[[nodiscard]] std::optional<std::filesystem::path> ResolveConfigPath(
    const std::optional<std::filesystem::path>& cli_override,
    const std::filesystem::path& home,
    const std::optional<std::filesystem::path>& xdg_config_home);

/// Produce a `Config` by overlaying the file at `user_config_path`
/// (if present) on top of the built-in defaults. Defaults live in
/// C++ so the parser stays usable in tests without reading
/// `data/config.default.toml` — which currently holds `[ingest]`,
/// `[detect]`, and `[focus]` tables that TB-01's parser rejects by
/// design.
///
/// `home` is used to expand leading `~` in path values and must be
/// non-empty whenever any resolved path contains one.
[[nodiscard]] Config LoadConfig(
    const std::optional<std::filesystem::path>& user_config_path,
    const std::filesystem::path& home);

/// Built-in compile-time defaults. Expands `~` using `home`. Split out
/// so the defaults are directly observable in tests without a
/// filesystem round-trip.
[[nodiscard]] Config BuiltInDefaults(const std::filesystem::path& home);

}  // namespace wildframe::cli
