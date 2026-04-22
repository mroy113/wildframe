# Wildframe Developer Setup

This document records how to build Wildframe. Full bootstrap automation
(Xcode command-line tools, Homebrew, LLVM for clang-tidy / clang-format,
vcpkg clone) lands under **S0-15**; this file currently captures the
toolchain minimums from **S0-20**.

---

## 1. Toolchain minimums

Recorded from `bird_photo_ai_project_handoff.md` §16.24 and enforced by
the top-level [CMakeLists.txt](../CMakeLists.txt). Building on a system
below any of these lines fails at CMake configure time with an error
message that points back here.

| Requirement | Minimum | How it is enforced |
|---|---|---|
| macOS | 13.0 (Ventura) | `CMAKE_OSX_DEPLOYMENT_TARGET` defaults to `13.0` before `project()` |
| Apple Clang | 15 (Xcode 15+) | `CMAKE_CXX_COMPILER_VERSION` gate after `project()` |
| CMake | 3.24 | `cmake_minimum_required(VERSION 3.24)` |
| Ninja | any recent release | the only generator wired in [CMakePresets.json](../CMakePresets.json) |
| vcpkg registry | `builtin-baseline` SHA in [vcpkg.json](../vcpkg.json) | resolved at configure time by vcpkg manifest mode |

The compiler gate is `APPLE`-guarded. On non-Apple hosts the gate is
skipped — vcpkg itself refuses to configure because [vcpkg.json](../vcpkg.json)
sets `"supports": "osx"`, so a non-macOS build stops earlier.

## 2. vcpkg pin strategy

The authoritative vcpkg pin is the `builtin-baseline` SHA in
[vcpkg.json](../vcpkg.json). That value fixes the vcpkg port registry
snapshot every dependency is resolved against, so a clean configure
picks the same versions on every machine.

A local vcpkg clone (the directory `VCPKG_ROOT` points at) must be on
the same SHA as the baseline. S0-15's bootstrap script will pin the
clone automatically; until then, a developer using an existing clone
should `git checkout <builtin-baseline>` in that directory before
running `cmake --preset ...` so `vcpkg install` resolves against the
same registry the manifest declares. If the two drift, the manifest
and the clone can produce different port versions without a visible
error.

## 3. What is not in this file yet

- Xcode command-line tools install (`xcode-select --install`).
- Homebrew install and LLVM (keg-only) for clang-tidy / clang-format.
- vcpkg clone and bootstrap pinned to the manifest baseline.
- First-build walkthrough (`cmake --preset debug`, `cmake --build --preset debug`, `ctest --preset debug`).
- `tools/bootstrap.sh` automating the above end-to-end.

All of that is scoped to **S0-15**. If setup friction surfaces before
S0-15 lands, capture it as a task note and cross-reference S0-15 so
the bootstrap script absorbs the fix.
