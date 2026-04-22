#pragma once

/// \file
/// Output-path helpers for Wildframe runtime artifacts (S0-19, FR-5,
/// FR-11). Two non-RAW outputs per run resolve through this header:
/// the spdlog daily-rotating application log and the per-batch JSON
/// manifest directory (M6-05). `docs/CONFIG.md` §3.1 pins the
/// user-visible defaults; this header is the implementation contract
/// those keys resolve against.
///
/// DI-shaped by design: every helper takes `home` as a parameter and
/// performs no environment reads. The startup wiring that resolves
/// `$HOME` into a path lives with the caller (S1-06 / M6-05), so the
/// helpers here stay trivially testable without touching process
/// environment.

#include <filesystem>

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

}  // namespace wildframe::orchestrator
