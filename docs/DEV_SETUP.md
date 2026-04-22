# Wildframe Developer Setup

This document describes how to take a clean macOS machine to a passing
Wildframe build, and the minimum toolchain the build depends on.

The fast path is [tools/bootstrap.sh](../tools/bootstrap.sh) (S0-15);
the rest of this document explains what the script does, why each step
is required, and how to perform the same steps by hand when that is
preferable.

---

## 1. Quickstart

From a fresh checkout of the repo:

```bash
./tools/bootstrap.sh
```

One command. Safe to re-run. The script installs what is missing and
skips what is present. On a clean machine expect a long first run —
the final step kicks off a cold vcpkg configure that builds Qt 6,
OpenCV, ONNX Runtime, Exiv2, and LibRaw from source.

When it finishes the script prints a one-line `export VCPKG_ROOT=...`
to add to your shell profile. This is the only manual follow-up; the
script does not edit shell configuration files.

## 2. Toolchain minimums

Recorded from `bird_photo_ai_project_handoff.md` §3 and §16.24 and
enforced by [cmake/PlatformMinimums.cmake](../cmake/PlatformMinimums.cmake),
which dispatches to [cmake/platforms/macos.cmake](../cmake/platforms/macos.cmake).
Building on a system below any of these lines fails at CMake configure
time with an error message that points back here.

| Requirement | Minimum | How it is enforced |
|---|---|---|
| macOS | 13.0 (Ventura) | `CMAKE_OSX_DEPLOYMENT_TARGET` defaults to `13.0` before `project()` |
| Apple Clang | 15 (Xcode 15+) | `CMAKE_CXX_COMPILER_VERSION` gate after `project()` |
| CMake | 3.24 | `cmake_minimum_required(VERSION 3.24)`; bootstrap.sh also asserts |
| Ninja | any recent release | the only generator wired in [CMakePresets.json](../CMakePresets.json) |
| vcpkg registry | `builtin-baseline` SHA in [vcpkg.json](../vcpkg.json) | resolved at configure time by vcpkg manifest mode |

The per-platform gate lives under [cmake/platforms/](../cmake/platforms/):
`macos.cmake` is populated; `linux.cmake` and `windows.cmake` are
stubs that error out at configure time (Linux/Windows are out of MVP
scope per [CLAUDE.md §6](../CLAUDE.md)). Cross-compile scoping is
tracked by **P2-01** in [docs/BACKLOG.md](BACKLOG.md).

## 3. What `tools/bootstrap.sh` installs

| Step | Tool(s) | Why |
|---|---|---|
| Xcode Command Line Tools | Apple Clang 15+, `git`, core unix utilities | Apple's compiler and git. The macOS default install does not include them; CLT does. |
| Homebrew | `brew` | Every other macOS dependency below lives in a Homebrew formula. |
| `cmake` | CMake ≥ 3.24 | Required build system. Gated by `cmake_minimum_required` in [CMakeLists.txt](../CMakeLists.txt) and by the script itself. |
| `ninja` | Ninja | [CMakePresets.json](../CMakePresets.json) hard-codes the Ninja generator. |
| `llvm` (keg-only) | clang-tidy, clang-format | Required by the NFR-7 (`format-check`) and NFR-8 (`tidy`) CI gates per [CONTRIBUTING.md](../CONTRIBUTING.md). Apple CLT does not ship either tool. [cmake/ClangTooling.cmake](../cmake/ClangTooling.cmake) hints both the Intel (`/usr/local/opt/llvm/bin`) and Apple Silicon (`/opt/homebrew/opt/llvm/bin`) keg prefixes. |
| `pkg-config` | `pkg-config` | Required by vcpkg ports that build through meson (e.g. `inih`, which is transitive through `exiv2`). Without it, a cold `cmake --preset debug` aborts partway through source-building dependencies. |
| `autoconf`, `autoconf-archive`, `automake`, `libtool` | autotools | Required by vcpkg ports that build through autotools. Mirrors the CI install list in [.github/workflows/ci.yml](../.github/workflows/ci.yml). |
| vcpkg clone | `$VCPKG_ROOT/vcpkg` | Dependency manager. Wildframe uses manifest mode ([vcpkg.json](../vcpkg.json)). The clone is pinned to the `builtin-baseline` SHA so one machine cannot silently resolve to a different port registry snapshot than another. |

## 4. vcpkg pin strategy

The authoritative vcpkg pin is the `builtin-baseline` SHA in
[vcpkg.json](../vcpkg.json). That value fixes the vcpkg port registry
snapshot every dependency is resolved against, so a clean configure
picks the same versions on every machine.

A local vcpkg clone (the directory `VCPKG_ROOT` points at) must be on
the same SHA as the baseline. The bootstrap script handles this
automatically — it runs `git checkout <builtin-baseline>` in the
clone after fetching. When changing vcpkg.json's pin, re-run
`tools/bootstrap.sh` (or `git -C "$VCPKG_ROOT" checkout <new-sha>`
by hand) so the clone follows.

If the two drift, the manifest and the clone can produce different
port versions without a visible error.

## 5. Manual setup (alternative to bootstrap.sh)

When the script is not appropriate — a partially-configured machine,
an environment where running third-party install scripts is not
permitted, an air-gapped network, or when debugging the script
itself — perform the same steps by hand.

```bash
# 1. Xcode Command Line Tools. The installer is GUI; wait for it to finish.
xcode-select --install

# 2. Homebrew. Follow the printed shell-env instructions at the end.
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 3. Build tooling.
brew install cmake ninja llvm pkg-config autoconf autoconf-archive automake libtool

# 4. vcpkg clone, pinned to the manifest baseline.
git clone https://github.com/microsoft/vcpkg "$HOME/vcpkg"
baseline=$(grep -E '"builtin-baseline"' vcpkg.json | sed -E 's/.*"([0-9a-f]{40})".*/\1/')
git -C "$HOME/vcpkg" checkout "$baseline"
"$HOME/vcpkg"/bootstrap-vcpkg.sh -disableMetrics

# 5. First configure + build.
export VCPKG_ROOT="$HOME/vcpkg"
cmake --preset debug
cmake --build --preset debug
```

If any of the above fail, the bootstrap script's output for the
equivalent step is the reference implementation — match its error
handling and re-run that step only.

## 6. Shell profile: `VCPKG_ROOT`

[CMakePresets.json](../CMakePresets.json) resolves the toolchain via
`$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`. With `VCPKG_ROOT`
unset, `cmake --preset <name>` fails at configure time.

Add one line to your shell profile:

```bash
# ~/.zshrc (macOS default)
export VCPKG_ROOT="$HOME/vcpkg"
```

Use the path `tools/bootstrap.sh` actually used if you exported a
different `VCPKG_ROOT` into the script's environment.

The bootstrap script **does not** write to the profile itself — silent
profile edits are out of character for a one-shot dev-env script.

## 7. Verifying the build

Once the profile is set, from a fresh shell:

```bash
cmake --build --preset debug        # incremental rebuild
ctest --preset debug                # test suite
cmake --build --preset debug --target format-check   # clang-format gate
cmake --build --preset tidy          # clang-tidy gate (NFR-8)
```

All of the above are the CI gates that run on every PR
(see [CONTRIBUTING.md](../CONTRIBUTING.md) "CI gates"). Passing them
locally is the minimum before opening a PR.

## 8. Notes and known footguns

- **Cold rebuilds are expensive.** `rm -rf build/<preset>/` forces
  vcpkg to rebuild every dependency from source — Qt 6 alone takes
  tens of minutes. Reuse warm build directories whenever possible.
- **Asset hosting.** [tools/fetch_models.cmake](../tools/fetch_models.cmake)
  and [tools/fetch_fixtures.cmake](../tools/fetch_fixtures.cmake)
  pull pinned URL + SHA256 pairs from this repo's GitHub Releases
  (`github.com/mroy113/wildframe/releases/`). If that repo is renamed,
  deleted, or becomes private, every clean configure and every CI run
  breaks. Flagged by the 2026-04-21 Sprint 0 infrastructure review
  §4; not automated away yet.
- **PATH quirks.** Homebrew on Intel macs installs to `/usr/local`;
  on Apple Silicon, to `/opt/homebrew`. The bootstrap script handles
  both. Agents running commands outside a login shell may find
  `/usr/local/bin` missing from PATH — prepend it explicitly.
- **Existing vcpkg clones with uncommitted work.** The bootstrap
  script refuses to check out a new SHA in a dirty vcpkg tree. Commit,
  stash, or point `VCPKG_ROOT` at a different directory before
  re-running.
