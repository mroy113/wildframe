#pragma once

/// \file
/// Output-path helpers for Wildframe runtime artifacts (S0-19, FR-5,
/// FR-11). Two non-RAW outputs per run resolve through this header:
/// the spdlog daily-rotating application log and the per-batch JSON
/// manifest directory (M6-05). `docs/CONFIG.md` §3.1 pins the
/// user-visible defaults; this header is the implementation contract
/// those keys resolve against.
///
/// DI-shaped by design: every pure helper takes `home` as a parameter
/// and performs no environment reads. Exactly one function —
/// `ResolveHomeFromEnv` — touches the process environment. Callers
/// invoke it once at startup and thread the result into every other
/// helper, so path resolution stays trivially testable without
/// mocking `getenv`.

#include <filesystem>
#include <optional>

namespace wildframe::orchestrator {

/// Expand a leading `~` in `p` to `home`, per `docs/CONFIG.md` §1.3.
/// Only bare `~` and `~/<rest>` expand; `~user/<rest>` is returned
/// unchanged because Wildframe does not resolve other users' home
/// directories. Non-tilde inputs (absolute, relative, empty) are
/// returned unchanged.
[[nodiscard]] std::filesystem::path ExpandTilde(
    const std::filesystem::path& p, const std::filesystem::path& home);

/// Default spdlog daily-rotating file-sink path. Resolves to
/// `<home>/Library/Logs/Wildframe/wildframe.log` per
/// `docs/CONFIG.md` §3.1 (`log_path`) and `docs/STYLE.md` §4.3.
/// Parent directories are created by the log init path, not here.
[[nodiscard]] std::filesystem::path DefaultLogPath(
    const std::filesystem::path& home);

/// Default directory for per-batch JSON manifests (M6-05). Resolves
/// to `<home>/Library/Application Support/Wildframe/batches` per
/// `docs/CONFIG.md` §3.1 (`manifest_dir`). Per-run filename
/// generation (`<timestamp>.json`) is M6-05's contract, not this
/// header's.
[[nodiscard]] std::filesystem::path DefaultManifestDir(
    const std::filesystem::path& home);

/// Read the user's home directory from the process environment.
/// Reads `HOME` on POSIX and `USERPROFILE` on Windows so the same
/// entry point works on macOS today (handoff §3) and on whatever
/// P2-01 decides later. Returns `std::nullopt` when the relevant
/// variable is unset or empty; the caller (application startup)
/// decides how to surface that to the user. Not marked `noexcept`
/// because `std::filesystem::path` construction may throw on
/// conversion failures.
[[nodiscard]] std::optional<std::filesystem::path> ResolveHomeFromEnv();

}  // namespace wildframe::orchestrator
