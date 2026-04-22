# Wildframe

Wildframe is a desktop application that scans folders of RAW bird
photographs and writes structured metadata back to each image:
whether a bird is present, whether it is in focus, and a keeper score
that helps triage a burst-heavy shoot.

Wildframe writes to **XMP sidecar files** (`<photo>.xmp` next to the
RAW), so the original RAW is never modified and any DAM tool that
reads XMP — Lightroom, digiKam, exiftool — can use Wildframe's results
without further integration.

## Status

Wildframe is **pre-MVP**. Most modules are still scaffolding; there is
no shippable binary yet. See [docs/BACKLOG.md](docs/BACKLOG.md) for
in-flight work and [bird_photo_ai_project_handoff.md](bird_photo_ai_project_handoff.md)
for the full scope and architecture.

This README is the user-facing entry point and will fill out as
modules land. For agent-facing operational rules see [CLAUDE.md](CLAUDE.md);
for contributor workflow see [CONTRIBUTING.md](CONTRIBUTING.md).

## Supported platform

| | |
|---|---|
| OS | macOS 13 (Ventura) or later |
| Architecture | Apple Silicon recommended; Intel supported |
| Input format | Canon CR3 RAW |

Linux, Windows, and RAW formats other than CR3 are out of scope for
the MVP. See [CLAUDE.md §6](CLAUDE.md) for the full out-of-scope list.

## Build

One command on a clean macOS machine:

```bash
./tools/bootstrap.sh
```

The script installs Xcode Command Line Tools, Homebrew, the build
toolchain (CMake, Ninja, LLVM, autotools), and a pinned vcpkg clone,
then runs the first `cmake --preset debug` + build. Expect a long
first run — vcpkg builds Qt 6, OpenCV, ONNX Runtime, Exiv2, and
LibRaw from source on a cold cache.

Per-step rationale, manual setup, and known footguns live in
[docs/DEV_SETUP.md](docs/DEV_SETUP.md).

## Run

Not yet wired. The CLI entry point lands with [M5 (orchestrator)](docs/BACKLOG.md);
the GUI lands with [M7 (Qt frontend)](docs/BACKLOG.md). This section
will document `wildframe` invocation once those modules are in place.

## Where Wildframe writes

| Output | Default location | Override |
|---|---|---|
| XMP sidecars | `<photo>.xmp` next to each RAW | not configurable; XMP-next-to-RAW is the contract DAM tools expect |
| Batch JSON manifest | `~/Library/Application Support/Wildframe/batches/<ISO-8601-timestamp>.json` | TOML key `manifest_dir` |
| Application log | `~/Library/Logs/Wildframe/wildframe.log` (daily rotation, 14-day retention) | TOML key `log_path` |

Defaults can be overridden via the runtime TOML config.
[docs/CONFIG.md](docs/CONFIG.md) is the full schema (key names, types,
ranges, defaults, resolution order). The XMP sidecar field schema
itself is documented in [docs/METADATA.md](docs/METADATA.md).

## Documentation map

| Reading | Audience |
|---|---|
| [bird_photo_ai_project_handoff.md](bird_photo_ai_project_handoff.md) | Source of truth for scope, architecture, FRs, and NFRs |
| [docs/DEV_SETUP.md](docs/DEV_SETUP.md) | Building from source on macOS |
| [docs/CONFIG.md](docs/CONFIG.md) | Runtime configuration schema |
| [docs/METADATA.md](docs/METADATA.md) | XMP sidecar field contract |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Module boundaries and pipeline shape |
| [docs/STYLE.md](docs/STYLE.md) | C++ style, exception policy, logging policy |
| [docs/LICENSING.md](docs/LICENSING.md) | License posture and dependency obligations |
| [docs/BACKLOG.md](docs/BACKLOG.md) | What's done, in flight, and on deck |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Branching, commits, PRs, CI gates |
| [CLAUDE.md](CLAUDE.md) | Operational manual for AI coding agents |

## License

Wildframe is distributed under the **GNU General Public License,
version 3.0 or (at your option) any later version**. See [LICENSE](LICENSE)
for the full text and [docs/LICENSING.md](docs/LICENSING.md) for the
posture, the obligations Qt 6's LGPL choice imposes, and the upstream
licenses of every statically linked dependency.
