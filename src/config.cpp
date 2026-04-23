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
#include <toml++/toml.hpp>

#include <array>
#include <algorithm>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>

#include "wildframe/orchestrator/paths.hpp"

namespace wildframe::cli {

namespace {

namespace wfo = ::wildframe::orchestrator;

// User-config filename under the XDG / macOS config directory. Fixed
// by `docs/CONFIG.md` §1.2 step 2.
constexpr std::string_view kUserConfigFilename = "config.toml";
constexpr std::string_view kUserConfigSubdir = "wildframe";

// macOS fallback directory when `$XDG_CONFIG_HOME` is unset, per
// `docs/CONFIG.md` §1.2 step 2. Matches `paths.hpp`'s layout style so
// the resolved absolute appears verbatim in test expectations.
constexpr std::string_view kMacConfigRelative =
    "Library/Application Support/Wildframe";

// Only key names with a live TB-01 consumer are parsed. Any other
// top-level key or any table triggers the §1.5 unknown-key rejection
// — each module's thickening pass widens this set.
constexpr std::array<std::string_view, 4> kAllowedTopLevelKeys = {
    "log_path",
    "log_level",
    "manifest_dir",
    "reanalysis_default",
};

// Compile-time default for `log_level` per `docs/STYLE.md` §4.2:
// debug / asan → "debug"; release → "info". `NDEBUG` is the standard
// way CMake's Release config signals release builds.
constexpr spdlog::level::level_enum kDefaultLogLevel =
#ifdef NDEBUG
    spdlog::level::info;
#else
    spdlog::level::debug;
#endif

// Accepted log_level values, in the docs/STYLE.md §4.1 order.
const std::unordered_map<std::string_view, spdlog::level::level_enum>&
LogLevelMap() {
  static const std::unordered_map<std::string_view, spdlog::level::level_enum>
      kMap{
          {"trace", spdlog::level::trace},
          {"debug", spdlog::level::debug},
          {"info", spdlog::level::info},
          {"warn", spdlog::level::warn},
          {"error", spdlog::level::err},
          {"critical", spdlog::level::critical},
      };
  return kMap;
}

const std::unordered_map<std::string_view, ReanalysisPolicy>&
ReanalysisPolicyMap() {
  static const std::unordered_map<std::string_view, ReanalysisPolicy> kMap{
      {"prompt", ReanalysisPolicy::kPrompt},
      {"skip", ReanalysisPolicy::kSkip},
      {"overwrite", ReanalysisPolicy::kOverwrite},
  };
  return kMap;
}

[[nodiscard]] bool IsAllowedTopLevelKey(std::string_view key) {
  return std::ranges::any_of(
      kAllowedTopLevelKeys,
      [key](std::string_view allowed) { return key == allowed; });
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
  return wfo::ExpandTilde(std::filesystem::path{raw_value}, home);
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
  const auto& map = LogLevelMap();
  if (const auto entry = map.find(raw); entry != map.end()) {
    out.log_level = entry->second;
    return;
  }
  std::ostringstream msg;
  msg << "config key 'log_level' has invalid value '" << raw
      << "'; expected one of: trace, debug, info, warn, error, critical "
         "(docs/CONFIG.md §3.1)";
  throw ConfigError{msg.str()};
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
  for (const auto& [raw_key, node] : tbl) {
    const std::string_view key{raw_key.str()};

    if (!IsAllowedTopLevelKey(key)) {
      std::ostringstream msg;
      msg << "config has unknown key '" << key
          << "' — typos fail loudly (docs/CONFIG.md §1.5). Only "
             "log_path, log_level, manifest_dir, reanalysis_default "
             "are recognized in the current build; [ingest], [detect], "
             "and [focus] tables land with their owning module tasks.";
      throw ConfigError{msg.str()};
    }

    if (key == "log_path") {
      ApplyLogPath(node, home, out);
    } else if (key == "log_level") {
      ApplyLogLevel(node, out);
    } else if (key == "manifest_dir") {
      ApplyManifestDir(node, home, out);
    } else if (key == "reanalysis_default") {
      ApplyReanalysisDefault(node, out);
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
    auto expanded = wfo::ExpandTilde(*cli_override, home);
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
  out.log_path = wfo::DefaultLogPath(home);
  out.manifest_dir = wfo::DefaultManifestDir(home);
  return out;
}

Config LoadConfig(
    const std::optional<std::filesystem::path>& user_config_path,
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
