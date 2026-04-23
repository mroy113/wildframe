#pragma once

/// \file
/// Wildframe CLI entry point (TB-01, FR-11). `main()` is a thin
/// wrapper around `Run()` so end-to-end tests can drive the CLI
/// in-process without spawning a subprocess (TB-09).
///
/// Exit codes are stable — they appear verbatim in `--help` output
/// and callers (scripts, tests) may branch on them.

#include <iosfwd>

namespace wildframe::cli {

/// Successful run or explicit action flag (`--help`, `--version`).
inline constexpr int kExitSuccess = 0;

/// Malformed command line (missing positional, unrecognized flag,
/// `--config` with no value). Corresponds to `UsageError`.
inline constexpr int kExitUsage = 2;

/// Configuration file failed validation (`docs/CONFIG.md` §5).
/// Corresponds to `ConfigError`.
inline constexpr int kExitConfig = 3;

/// Runtime failure that was not a configuration problem — reserved
/// for the M6-04 "partial batch failure" surface once the
/// orchestrator (TB-02) is wired. TB-01 maps any uncaught exception
/// here.
inline constexpr int kExitRuntime = 4;

/// Drive one CLI invocation. Parses argv, resolves and loads the
/// TOML config, initializes the log, and (once TB-02 lands) runs
/// the pipeline. `err` receives one-line diagnostics on failure —
/// the only stderr use in Wildframe per `docs/STYLE.md` §4.3 and
/// `docs/CONFIG.md` §5.
///
/// Help / version text is written to `stdout`, not `err`.
[[nodiscard]] int Run(int argc, const char* const* argv, std::ostream& err);

}  // namespace wildframe::cli
