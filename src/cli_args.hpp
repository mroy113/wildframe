#pragma once

/// \file
/// Hand-rolled argv parser for the Wildframe CLI (TB-01, FR-11).
///
/// Recognizes exactly one positional argument (the RAW input directory),
/// an optional `--config <path>`, and two action flags `--help` /
/// `--version`. A new CLI-parsing dependency is not justified under
/// NFR-6 for this surface — the handoff doc does not authorize one.
///
/// Exception policy per `docs/STYLE.md` §3: malformed invocations throw
/// `UsageError`, which `Run()` translates into a non-zero exit code
/// and a one-line stderr diagnostic.

#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace wildframe::cli {

/// What `Run()` should do after args parse successfully. `--help` and
/// `--version` exit early; a bare invocation with a directory runs the
/// pipeline.
enum class Action : std::uint8_t {
  kRun,
  kShowHelp,
  kShowVersion,
};

/// Parsed-argv value type. Populated by `ParseArgs` and consumed by
/// `Run()`. Public `snake_case` members per `docs/STYLE.md` §2.1
/// (plain aggregate, Rule of Zero).
struct ParsedArgs {
  Action action = Action::kRun;

  /// `--config <path>` value. Empty when the user did not pass it;
  /// `Run()` then resolves the config per `docs/CONFIG.md` §1.2.
  std::optional<std::filesystem::path> config_path;

  /// Positional RAW-input directory. Populated only when `action ==
  /// kRun`; ignored for `--help` / `--version`.
  std::filesystem::path directory;
};

/// Thrown by `ParseArgs` on any malformed invocation: missing
/// positional, extra positional, unrecognized flag, missing value for
/// `--config`. Translated to exit code `kExitUsage` by `Run()`.
class UsageError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/// Parse `argv`. `argv[0]` is the program name and is ignored by the
/// parser. Throws `UsageError` on malformed input.
[[nodiscard]] ParsedArgs ParseArgs(int argc, const char* const* argv);

/// Usage text emitted by `--help`. Stable string for testing.
[[nodiscard]] std::string_view UsageText() noexcept;

/// Version text emitted by `--version`. Matches the CMake project
/// version; updates land in [CMakeLists.txt](../../CMakeLists.txt).
[[nodiscard]] std::string_view VersionText() noexcept;

}  // namespace wildframe::cli
