#include "config.hpp"

#include <spdlog/common.h>

// tomlplusplus exposes its public surface through the umbrella
// `toml++/toml.hpp`; its sub-headers are implementation details, but
// `misc-include-cleaner` does not ship a mapping and cannot infer
// the umbrella. Scope the suppression to the include block and the
// translation unit's toml::* references so we do not leak NOLINTs
// onto every line. Alternative real fixes attempted: (a) including
// each `toml++/impl/*.hpp` directly — rejected because those are
// documented as non-stable; (b) introducing a Wildframe wrapper
// layer over toml — rejected per STYLE §2.9 (no decoupling,
// narrowing, or invariant addition to justify the maintenance
// surface).
// NOLINTBEGIN(misc-include-cleaner)
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <toml++/toml.hpp>
#include <unordered_map>

#include "wildframe/orchestrator/paths.hpp"

namespace wildframe::cli {

namespace {

// User-config filename under the XDG / macOS config directory. Fixed
// by `docs/CONFIG.md` §1.2 step 2.
constexpr std::string_view kUserConfigFilename = "config.toml";
constexpr std::string_view kUserConfigSubdir = "wildframe";

// macOS fallback directory when `$XDG_CONFIG_HOME` is unset, per
// `docs/CONFIG.md` §1.2 step 2. Matches `paths.hpp`'s layout style so
// the resolved absolute appears verbatim in test expectations.
constexpr std::string_view kMacConfigRelative =
    "Library/Application Support/Wildframe";

// Compile-time default for `log_level` per `docs/STYLE.md` §4.2:
// debug / asan → "debug"; release → "info". `NDEBUG` is the standard
// way CMake's Release config signals release builds.
constexpr spdlog::level::level_enum kDefaultLogLevel =
#ifdef NDEBUG
    spdlog::level::info;
#else
    spdlog::level::debug;
#endif

const std::unordered_map<std::string_view, ReanalysisPolicy>&
ReanalysisPolicyMap() {
  static const std::unordered_map<std::string_view, ReanalysisPolicy> kMap{
      {"prompt", ReanalysisPolicy::kPrompt},
      {"skip", ReanalysisPolicy::kSkip},
      {"overwrite", ReanalysisPolicy::kOverwrite},
  };
  return kMap;
}

[[nodiscard]] std::filesystem::path ExpandPathValue(
    std::string_view raw_value, const std::filesystem::path& home,
    std::string_view key_name) {
  if (raw_value.starts_with('~') &&
      (raw_value == "~" || raw_value.starts_with("~/"))) {
    if (home.empty()) {
      std::ostringstream msg;
      msg << "config key '" << key_name
          << "' uses '~' but $HOME is unset or empty "
             "(docs/CONFIG.md §5)";
      throw ConfigError{msg.str()};
    }
  }
  return wildframe::orchestrator::ExpandTilde(std::filesystem::path{raw_value},
                                              home);
}

[[nodiscard]] std::string RequireStringValue(const toml::node& node,
                                             std::string_view key_name) {
  if (const auto* value = node.as_string(); value != nullptr) {
    return std::string{value->get()};
  }
  std::ostringstream msg;
  msg << "config key '" << key_name
      << "' must be a string (docs/CONFIG.md §1.6)";
  throw ConfigError{msg.str()};
}

void ApplyLogPath(const toml::node& node, const std::filesystem::path& home,
                  Config& out) {
  const auto raw = RequireStringValue(node, "log_path");
  out.log_path = ExpandPathValue(raw, home, "log_path");
}

void ApplyManifestDir(const toml::node& node, const std::filesystem::path& home,
                      Config& out) {
  const auto raw = RequireStringValue(node, "manifest_dir");
  out.manifest_dir = ExpandPathValue(raw, home, "manifest_dir");
}

void ApplyLogLevel(const toml::node& node, Config& out) {
  const auto raw = RequireStringValue(node, "log_level");
  // Delegate the string-to-level mapping to spdlog itself — we use
  // spdlog's canonical names (`warning`, `error`, ...) per
  // docs/CONFIG.md §3.1. `from_str` returns `off` on any unknown
  // input; we also reject an explicit `"off"` because silencing
  // logs entirely would undermine the NFR-5 audit trail.
  const auto level = spdlog::level::from_str(raw);
  if (level == spdlog::level::off) {
    std::ostringstream msg;
    msg << "config key 'log_level' has invalid value '" << raw
        << "'; expected one of: trace, debug, info, warning, error, critical "
           "(docs/CONFIG.md §3.1)";
    throw ConfigError{msg.str()};
  }
  out.log_level = level;
}

void ApplyReanalysisDefault(const toml::node& node, Config& out) {
  const auto raw = RequireStringValue(node, "reanalysis_default");
  const auto& map = ReanalysisPolicyMap();
  if (const auto entry = map.find(raw); entry != map.end()) {
    out.reanalysis_default = entry->second;
    return;
  }
  std::ostringstream msg;
  msg << "config key 'reanalysis_default' has invalid value '" << raw
      << "'; expected one of: prompt, skip, overwrite (docs/CONFIG.md §3.1)";
  throw ConfigError{msg.str()};
}

void ApplyUserConfig(const toml::table& tbl, const std::filesystem::path& home,
                     Config& out) {
  // The if-chain is the single source of truth for recognized keys.
  // Any TOML entry that does not match a branch falls into the
  // unknown-key rejection below (docs/CONFIG.md §1.5). When a module
  // thickens its config surface (M1-02 done; M3-*, M4-*), that PR
  // adds a branch here in the same commit that parses its table.
  for (const auto& [raw_key, node] : tbl) {
    const std::string_view key{raw_key.str()};

    if (key == "log_path") {
      ApplyLogPath(node, home, out);
    } else if (key == "log_level") {
      ApplyLogLevel(node, out);
    } else if (key == "manifest_dir") {
      ApplyManifestDir(node, home, out);
    } else if (key == "reanalysis_default") {
      ApplyReanalysisDefault(node, out);
    } else {
      std::ostringstream msg;
      msg << "config has unknown key '" << key
          << "' — typos fail loudly (docs/CONFIG.md §1.5). Only "
             "log_path, log_level, manifest_dir, reanalysis_default "
             "are recognized in the current build; [ingest], [detect], "
             "and [focus] tables land with their owning module tasks.";
      throw ConfigError{msg.str()};
    }
  }
}

}  // namespace

std::optional<std::filesystem::path> ResolveConfigPath(
    const std::optional<std::filesystem::path>& cli_override,
    const std::filesystem::path& home,
    const std::optional<std::filesystem::path>& xdg_config_home) {
  if (cli_override.has_value()) {
    // Tilde-expand per docs/CONFIG.md §1.3, then require existence:
    // "used as-is. A missing file here is a startup error". Kept
    // non-const so the value moves out on return.
    auto expanded = wildframe::orchestrator::ExpandTilde(*cli_override, home);
    if (expanded.string().starts_with('~')) {
      throw ConfigError{
          "--config path uses '~' but $HOME is unset or empty "
          "(docs/CONFIG.md §5)"};
    }
    std::error_code ec;
    if (!std::filesystem::exists(expanded, ec)) {
      std::ostringstream msg;
      msg << "--config path does not exist: " << expanded.string()
          << " (docs/CONFIG.md §5)";
      throw ConfigError{msg.str()};
    }
    return expanded;
  }

  std::error_code ec;

  if (xdg_config_home.has_value() && !xdg_config_home->empty()) {
    auto candidate = *xdg_config_home / kUserConfigSubdir / kUserConfigFilename;
    if (std::filesystem::exists(candidate, ec)) {
      return candidate;
    }
  }

  if (!home.empty()) {
    auto candidate = home / kMacConfigRelative / kUserConfigFilename;
    if (std::filesystem::exists(candidate, ec)) {
      return candidate;
    }
  }

  return std::nullopt;
}

Config BuiltInDefaults(const std::filesystem::path& home) {
  Config out;
  out.log_level = kDefaultLogLevel;
  out.reanalysis_default = ReanalysisPolicy::kPrompt;
  // `DefaultLogPath` / `DefaultManifestDir` would return a path rooted
  // at an empty prefix when `home` is empty; that later fails at the
  // log-sink layer with an opaque message. Fail here instead.
  if (home.empty()) {
    throw ConfigError{
        "$HOME is unset or empty; cannot resolve default log_path or "
        "manifest_dir (docs/CONFIG.md §5)"};
  }
  out.log_path = wildframe::orchestrator::DefaultLogPath(home);
  out.manifest_dir = wildframe::orchestrator::DefaultManifestDir(home);
  return out;
}

Config LoadConfig(const std::optional<std::filesystem::path>& user_config_path,
                  const std::filesystem::path& home) {
  Config out = BuiltInDefaults(home);

  if (!user_config_path.has_value()) {
    return out;
  }

  toml::table tbl;
  try {
    tbl = toml::parse_file(user_config_path->string());
  } catch (const toml::parse_error& e) {
    std::ostringstream msg;
    msg << "config file " << user_config_path->string()
        << " has a TOML syntax error: " << e.description();
    if (const auto& source = e.source();
        source.begin != toml::source_position{}) {
      msg << " at line " << source.begin.line << ", column "
          << source.begin.column;
    }
    throw ConfigError{msg.str()};
  }

  ApplyUserConfig(tbl, home, out);
  return out;
}

}  // namespace wildframe::cli
// NOLINTEND(misc-include-cleaner)
