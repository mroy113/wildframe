# Wildframe MVP Backlog

Ordered task decomposition for Wildframe MVP. Each task is atomic, has explicit dependencies, and cites the FR/NFR it satisfies from `bird_photo_ai_project_handoff.md`.

**Legend:**
- **ID**: task identifier (stable, referenced from commits/PRs).
- **Deps**: task IDs that must be complete first.
- **Size**: S (<1 day), M (1–3 days), L (3–7 days).
- **Satisfies**: FR-x / NFR-x from the handoff doc.

**Rules:**
- Do tasks in ID order within a phase unless dependencies dictate otherwise.
- Do not begin a task until its dependencies are merged to `main`.
- Flip a task's checkbox to `[x]` inside its implementation PR. If the PR doesn't merge, the checkbox flip doesn't either — no separate bookkeeping PR.

---

## Sprint 0 — Scaffolding (blocks everything else)

- [x] **S0-01** — Pitchfork directory scaffold
  - Deps: none
  - Size: S
  - Satisfies: §11 (layout)
  - Create empty directory tree matching the handoff doc §11. Add placeholder `CMakeLists.txt` files where shown. Commit `.gitignore` (build/, .DS_Store, IDE files, `build/_models/`).

- [x] **S0-02** — `vcpkg.json` manifest with pinned versions
  - Deps: S0-01
  - Size: S
  - Satisfies: §5 (tech stack), NFR-6
  - Add manifest mode `vcpkg.json` at repo root. Pin: libraw, exiv2, opencv, onnxruntime, qt6-base, qt6-widgets, nlohmann-json, spdlog, gtest. Document exact pinned versions in the PR body.

- [x] **S0-03** — Top-level `CMakeLists.txt` with empty static-lib targets
  - Deps: S0-02
  - Size: M
  - Satisfies: §10, §11, NFR-6
  - One `add_library(wildframe_<module> STATIC)` per MVP module, each with an empty `.cpp` and a public header. One `add_executable(wildframe)` for the GUI. Wire vcpkg toolchain. Must build clean with no sources.

- [x] **S0-04** — `CMakePresets.json` with debug / release / tidy / asan
  - Deps: S0-03
  - Size: S
  - Satisfies: §3 (delivery), NFR-8
  - `debug`: `-DCMAKE_BUILD_TYPE=Debug -g`. `release`: `-DCMAKE_BUILD_TYPE=Release -O3`. `tidy`: runs clang-tidy over the compile-commands database. `asan`: debug + AddressSanitizer.

- [x] **S0-05** — `.clang-format` baseline
  - Deps: S0-03
  - Size: S
  - Satisfies: NFR-7
  - Derive from Google style. Document deviations in `docs/STYLE.md`. Add `format` and `format-check` CMake targets.

- [x] **S0-06** — `.clang-tidy` baseline and tuning
  - Deps: S0-05
  - Size: M
  - Satisfies: NFR-8
  - Enable the check set in NFR-8. Run against a representative sample (write a 200-line throwaway module). Disable only checks that are unequivocally noisy; document each disabled check inline in `.clang-tidy` with a one-line justification.

- [x] **S0-07** — `docs/STYLE.md`
  - Deps: S0-01
  - Size: M
  - Satisfies: NFR-7
  - Resolve Core Guidelines vs Google Style conflicts for: exception policy (per handoff §NFR-7), header ordering, smart-pointer conventions, naming (CamelCase vs snake_case for types/functions/variables), use of `auto`, RAII patterns, `noexcept` policy. Every resolution cites why.

- [x] **S0-08** — `docs/LICENSING.md`
  - Deps: S0-01
  - Size: S
  - Satisfies: §15 (risks)
  - Document Qt 6 LGPL + dynamic-linking obligation, Wildframe's license choice (needs customer decision), upstream licenses for LibRaw, Exiv2, OpenCV, ONNX Runtime, spdlog, nlohmann/json, GoogleTest. Document model-weight URLs + upstream licenses for YOLOv11 and MegaDetector.

- [x] **S0-09** — `docs/ARCHITECTURE.md`
  - Deps: S0-01
  - Size: M
  - Satisfies: §10, NFR-3
  - Module dependency graph (Mermaid or ASCII). Per-module one-paragraph purpose. Data-flow walkthrough (from §12 of the handoff). Stage-registration extension point for Phase 2+.

- [x] **S0-10** — `docs/METADATA.md`
  - Deps: S0-01
  - Size: S
  - Satisfies: FR-5, FR-8, NFR-4
  - Document every field in the three namespaces (`wildframe:`, `wildframe_provenance:`, `wildframe_user:`). For each: name, type, range, whether AI- or user-written, and a one-line purpose. This file is the contract external tools (Lightroom smart collections) rely on.

- [x] **S0-11** — `tools/fetch_models.cmake`
  - Deps: S0-03
  - Size: M
  - Satisfies: §11 (model assets), §15 (licensing)
  - CMake module called during configure. Downloads YOLOv11 ONNX from a pinned URL to `build/_models/yolov11.onnx`. Verifies SHA256. Caches on re-run. Idempotent. Fails configure with a clear message if the download fails.

- [x] **S0-12** — Test fixture policy + initial CR3 fixture set
  - Deps: S0-01
  - Size: M
  - Satisfies: NFR-2
  - Customer decision recorded: fetch at configure time, mirroring S0-11's models pattern. Policy documented in `docs/FIXTURES.md`. `tools/fetch_fixtures.cmake` pins fixtures by URL + SHA256 with an idempotent cache. Fixtures acquired for categories (1) clear bird, (2) no bird, (3) small distant bird. Categories (4) motion-blurred bird and (5) non-bird false-positive magnet are split to **S0-12b** — customer did not have matching CR3s on hand.

- [ ] **S0-12b** — Acquire remaining CR3 fixtures (motion-blurred bird, false-positive magnet)
  - Deps: S0-12
  - Size: S
  - Satisfies: NFR-2
  - **Agent directive: do not pick up this task unless the customer explicitly asks for it by ID.** Unblocking requires the customer to provide the source CR3s; an agent starting this task without that input will stall. When the customer signals readiness, acquire CR3s for category (4) motion-blurred bird (visible bird large enough to detect, clearly motion-blurred — low Laplacian variance and low FFT high-frequency energy) and category (5) non-bird false-positive magnet (bird statue, sign, decoy, or carving that a COCO-trained YOLO could plausibly classify as "bird"). Upload as assets to a `fixtures-vN+1` release, pin in `tools/fetch_fixtures.cmake`, update `docs/FIXTURES.md` §2 with filename, size, intent, and dependent-test list. Acceptance criteria detailed in `docs/FIXTURES.md` §4.

- [x] **S0-13** — CI: GitHub Actions, macOS runner
  - Deps: S0-04, S0-05, S0-06
  - Size: M
  - Satisfies: NFR-7, NFR-8
  - Jobs: configure + build (debug and release), run `ctest`, run format-check, run clang-tidy gate. All required to pass before merge. Cache vcpkg and `build/_models/`.

- [x] **S0-14** — spdlog global logger initialization
  - Deps: S0-02, S0-03, S0-17
  - Size: S
  - Satisfies: NFR-5 (auditability)
  - Define a `wildframe_log` library or inline header in `wildframe_orchestrator` providing a configured spdlog sink (stdout + rotating file). Log format includes timestamp, level, and module tag. Level thresholds and destinations per S0-17.

- [x] **S0-15** — Developer environment bootstrap
  - Deps: S0-01, S0-02
  - Size: M
  - Satisfies: NFR-6 (maintainability)
  - `docs/DEV_SETUP.md` + a `tools/bootstrap.sh` script covering: Xcode command-line tools, Homebrew, CMake ≥3.24, Ninja, vcpkg clone + bootstrap (pinned to a specific commit for reproducibility), first `cmake --preset debug` + build. Must take a clean macOS machine to "first build passing" with one command.

- [x] **S0-16** — User-facing README.md
  - Deps: S0-01
  - Size: S
  - Satisfies: NFR-6
  - Repo-root `README.md` that explains what Wildframe is, who it's for, how to build and run it, and where to find deeper docs. Must document:
    - Supported platform (macOS 13+, Apple Silicon recommended).
    - Where XMP sidecars are written (next to the RAW file).
    - Where batch JSON manifests are written (`~/Library/Application Support/Wildframe/batches/`).
    - Where application logs are written (`~/Library/Logs/Wildframe/wildframe.log`, daily rotation, 14-day retention).
    - How to override any of these via the TOML config (pointer to `docs/CONFIG.md`).
    - How to build (pointer to `docs/DEV_SETUP.md`).
  - Separate from `CLAUDE.md` (agent-facing) and `CONTRIBUTING.md` (contributor-facing). Initial sections can be stubs, filled in as modules land.

- [x] **S0-17** — Logging policy
  - Deps: S0-01
  - Size: S
  - Satisfies: NFR-5
  - Section added to `docs/STYLE.md` specifying: level semantics (trace/debug/info/warn/error/critical), when to use each, default level for release vs debug builds, log file path and rotation policy (consumes S0-19 output), module tag conventions. Agents follow this spec when adding log lines.

- [x] **S0-18** — TOML runtime config specification
  - Deps: S0-01, S0-02
  - Size: M
  - Satisfies: FR-11
  - `docs/CONFIG.md` documenting every configurable field: name, type, range, default, which FR/NFR it relates to. Ship a fully-commented default `data/config.default.toml`. Specify the resolution order (CLI arg → XDG path → built-in default). Config is startup-only for MVP — no hot-reload.

- [x] **S0-19** — Output path implementation
  - Deps: S0-18
  - Size: S
  - Satisfies: FR-5, FR-11
  - **Locked defaults:**
    - spdlog application log: `~/Library/Logs/Wildframe/wildframe.log` with daily rotation, 14-day retention.
    - Batch JSON manifest: `~/Library/Application Support/Wildframe/batches/<ISO-8601-timestamp>.json`.
    - Both paths are overridable via TOML config keys `log_path` and `manifest_dir`.
  - Implement path-resolution helpers. Record in `docs/CONFIG.md`. Cross-link from `README.md` (see S0-16).

- [x] **S0-20** — macOS deployment target + toolchain minimums
  - Deps: S0-01
  - Size: S
  - Satisfies: NFR-6 (reproducibility), §3 (platform)
  - **Decided:** minimum macOS **13 (Ventura)**. Minimum Apple Clang: 15. Minimum CMake: 3.24. Ninja required. vcpkg pinned to a specific commit. Enforced by top-level `CMakeLists.txt` via `CMAKE_OSX_DEPLOYMENT_TARGET=13.0` and compiler version check. Record in `docs/DEV_SETUP.md`.

- [x] **S0-21** — Re-analysis behavior
  - Deps: S0-01
  - Size: S
  - Satisfies: FR-10
  - **Decided:** default is **prompt-per-batch** with three batch-level options (*skip already-analyzed*, *overwrite all*, *cancel*) and a per-file override available from the detail view. Chosen option recorded in the batch manifest and on each affected sidecar via `wildframe_user:reanalysis_policy_used`. Overridable via TOML config key `reanalysis_default` for headless/automated runs. Policy surface already landed across [bird_photo_ai_project_handoff.md §FR-10](../bird_photo_ai_project_handoff.md), [docs/CONFIG.md §3.1](CONFIG.md) (`reanalysis_default`), [docs/METADATA.md §5](METADATA.md) (`wildframe_user:reanalysis_policy_used`), [docs/ARCHITECTURE.md §3.1](ARCHITECTURE.md), and [data/config.default.toml](../data/config.default.toml) — implementation consumers are M6-05 (manifest) and M7-10 (prompt dialog), which depend on S0-21 + M5-09.

- [x] **S0-22** — `LICENSE` file + `vcpkg.json` license field
  - Deps: S0-08
  - Size: S
  - Satisfies: §15 (risks, licensing posture)
  - Per [docs/LICENSING.md §1.1](LICENSING.md), the GPL-3.0-or-later decision is recorded in prose but not machine-readable. Add the full GPL-3.0-or-later license text at repo root as `LICENSE`, and set `"license": "GPL-3.0-or-later"` in `vcpkg.json` (currently `null`). Identified by the 2026-04-21 Sprint 0 infrastructure review §5.

- [x] **S0-23** — ARCHITECTURE.md catch-up for `wildframe_log`
  - Deps: S0-14
  - Size: S
  - Satisfies: NFR-6 (doc/code coupling — architecture doc is the entry point for new contributors)
  - S0-14 landed `wildframe_log` as an 8th module (sole owner of spdlog), but [docs/ARCHITECTURE.md](ARCHITECTURE.md) still described seven modules, attributed spdlog to `wildframe_orchestrator` in the boundary table, and §6 told every module to use spdlog directly. Update the mermaid graph, boundary table, §2 module-purpose list, and §6 out-of-band paragraph to reflect the indirection. Identified by the 2026-04-23 main-state review §1.

- [x] **S0-24** — CLAUDE.md "Where to find things" table catch-up
  - Deps: S0-12, S0-15, S0-16, S0-18
  - Size: S
  - Satisfies: NFR-6 (discoverability — CLAUDE.md is the first file agents read)
  - [docs/CONFIG.md](CONFIG.md) (S0-18), [docs/FIXTURES.md](FIXTURES.md) (S0-12), [docs/DEV_SETUP.md](DEV_SETUP.md) (S0-15), and [README.md](../README.md) (S0-16) all landed without being added to the [CLAUDE.md](../CLAUDE.md) §2 table. Add one row per doc. Identified by the 2026-04-23 main-state review §1.

---

## Sprint 1 — Infrastructure follow-ups

Not part of the Sprint 0 scaffolding contract, not gating any module work. Lives here to stay visible and get picked up once module work begins to feel the friction.

- [x] **S1-01** — Cache `build/_vcpkg_installed/` directly in CI
  - Deps: S0-13
  - Size: S
  - Satisfies: NFR-6 (developer ergonomics — CI latency drives review cadence)
  - Landed in the same PR as S0-13 after the first green run clocked in at ~3 h wall time — well past the "~45 min friction" trigger. See [.github/workflows/ci.yml](../.github/workflows/ci.yml): the `Cache vcpkg_installed tree` step keys on `vcpkg.json` plus the S0-20 platform-minimums CMake files, with `restore-keys` allowing a partial hit when only `vcpkg.json` changes so vcpkg reconciles the delta via `x-gha` instead of rebuilding Qt 6 from scratch.

- [x] **S1-02** — CI job: AddressSanitizer + UndefinedBehaviorSanitizer
  - Deps: S0-13, M1-05 (needs at least one real test suite to exercise — otherwise the job passes trivially)
  - Size: S
  - Satisfies: NFR-6 (dynamic memory-safety coverage complementing the clang-tidy static gate)
  - The `asan` preset in [CMakePresets.json](../CMakePresets.json) is "Local use; not wired into CI by S0-04" — dynamic-analysis coverage in CI is currently zero. Add a sibling preset `asan-ubsan` (or extend `asan`) that passes `-fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer` and a matching workflow job that runs `ctest --preset asan-ubsan`. Treat any sanitizer error as a failing check. Runtime overhead is ~2× for ASan and negligible for UBSan; expect the job to roughly double the project-build+test portion of CI (not the vcpkg portion, which is shared). Scope excludes LeakSanitizer until we have enough lifetime-managed state to make it actionable (keep the `LSAN_OPTIONS=detect_leaks=0` escape hatch if the tool defaults it on). **Landed:** extended the existing `asan` preset in-place (rather than a sibling) since it had no callers; kept the `asan` name to avoid drift with handoff-doc mentions. Dep proceeded despite M1-05 unchecked because S0-14 (log tests) and S0-19 (orchestrator paths tests) supply the real test surface the dep's rationale required. Customer overrode the task's "scope excludes LSan" clause — left `detect_leaks` at its default rather than force-disabling, so leak detection engages automatically if a future Linux CI job lands.

- [ ] **S1-03** — CI job: ThreadSanitizer
  - Deps: M6-03 (no application threads exist before the orchestrator's worker)
  - Size: S
  - Satisfies: NFR-6, handoff §5 threading strategy
  - Wildframe's runtime has exactly two application-owned threads per [docs/ARCHITECTURE.md §5](ARCHITECTURE.md#5-threading-model): Qt main + one orchestrator worker, with a job queue and cancel flag across them. That is precisely the surface TSan exists to cover. Add a `tsan` preset (`-fsanitize=thread -fno-omit-frame-pointer`) and a CI job that runs the orchestrator's integration tests (M6-07) under it. **Do not start before M6-03** — without a real worker thread the job is a no-op. Known constraints: TSan is incompatible with ASan (separate job), and Qt's own instrumentation may surface benign warnings — triage into a small per-test suppression list rather than globally disabling checks.

- [x] **S1-04** — Run clang-tidy from debug's `compile_commands.json` instead of a dedicated preset
  - Deps: S0-13, S1-01
  - Size: S
  - Satisfies: NFR-6 (CI latency), NFR-8 (static-analysis gate)
  - Landed in the same PR as S0-13 / S1-01. The `Clang-tidy gate` step in [.github/workflows/ci.yml](../.github/workflows/ci.yml) now invokes `run-clang-tidy -p build/debug`, reusing debug's `compile_commands.json` rather than reconfiguring and recompiling the project under the `tidy` preset. The `tidy` preset remains in [CMakePresets.json](../CMakePresets.json) for local editor-loop use where inline tidying is handier than a post-hoc sweep.

- [x] **S1-05** — Move `format-check` before the first expensive build for fail-fast
  - Deps: S0-13
  - Size: XS
  - Satisfies: NFR-6 (developer ergonomics), NFR-7 (formatter)
  - Landed in the same PR as S0-13. `format-check` now runs immediately after `Configure (debug)`, before `Build (debug)` / `Test (debug)`. A formatting violation surfaces in seconds instead of after a full debug build + test pass.

- [ ] **S1-06** — Split CI into parallel jobs (debug / release / tidy fan-out)
  - Deps: S0-13, S1-01
  - Size: M
  - Satisfies: NFR-6 (CI latency)
  - Today [.github/workflows/ci.yml](../.github/workflows/ci.yml) is a single `macos-14` job that runs every step sequentially. The natural fan-out is a `setup` job (checkout + brew + vcpkg bootstrap + cache prime) that three downstream jobs `needs:` — **debug** (format-check + build + test), **release** (configure + build), **tidy** (`run-clang-tidy -p build/debug`, which first needs its own `configure (debug)` to regenerate `compile_commands.json` because the filesystem doesn't persist across GH runners). Each parallel job restores the `vcpkg_installed` cache independently. **Don't pull the trigger yet** — currently the project compiles in under a minute and tidy runs in seconds, so each split would pay its own ~3–5 min setup tax (cache restore of the 3–5 GB tree + brew install + vcpkg clone) that exceeds the concurrent savings. Revisit once sequential `build(debug) + build(release) + run-clang-tidy` wall time crosses ~15 min of actual work. Known sharp edges: the shared `x-gha` binary cache can race on a cold vcpkg.json change (multiple jobs rebuild the same port simultaneously) — if that becomes an issue, the `setup` job primes the binary cache before fan-out.

- [x] **S1-08** — Promote `VCPKG_INSTALLED_DIR` into CMakePresets.json `_base` preset
  - Deps: S0-04, S1-01
  - Size: S
  - Satisfies: NFR-6 (CI latency, local/CI parity, developer ergonomics)
  - S1-01 landed the `Cache vcpkg_installed tree` step against `build/_vcpkg_installed`, relying on the `VCPKG_INSTALLED_DIR` env var at the job level in [.github/workflows/ci.yml](../.github/workflows/ci.yml) to redirect vcpkg's install tree there. Observation from the first post-S1-01 run ([#24782789092](https://github.com/mroy113/wildframe/actions/runs/24782789092)): vcpkg's CMake toolchain integration reads `VCPKG_INSTALLED_DIR` from the CMake cache, not the process environment, so the env var was silently ignored — ports landed in `build/debug/vcpkg_installed/` and `build/release/vcpkg_installed/` per preset-default, `build/_vcpkg_installed/` was never created, and the post-step warned `Path Validation Error: Path(s) specified in the action for caching do(es) not exist, hence no cache is being saved`. Fixed by moving `VCPKG_INSTALLED_DIR` into the [CMakePresets.json](../CMakePresets.json) `_base` preset's `cacheVariables` block so every inheriting preset sees the same install tree both in CI and locally; the ci.yml env var was removed and replaced with a comment pointing at the preset. Filed and addressed in the S0-13 + S1-01/04/05 PR. Identified by the 2026-04-22 CI-hardening review §1 / §5.

---

## Sprint 2 — Tracer-bullet vertical slice

One command produces a valid XMP sidecar + batch manifest row for every CR3 in a chosen directory, end-to-end, with every stage wired. Expensive stages (raw decode, detection, focus, AI-namespace XMP writes) ship as **stubs** that satisfy their public interfaces with sentinel returns; cheap stages (ingest, orchestrator skeleton, provenance-only XMP write, minimal batch manifest) ship real. Subsequent module sections replace each stub with its real implementation while keeping `TB-09` (end-to-end smoke) green. Strategy per [bird_photo_ai_project_handoff.md §18](../bird_photo_ai_project_handoff.md).

Stubs are not TODOs. Every `TB-*` task lands with full Definition of Done (clang-format, clang-tidy zero findings, tests that pin the sentinel behavior). See [docs/STYLE.md §2.11](STYLE.md) — stubs have a live caller (the CLI) from the moment they ship, so they are not "speculative interface surface."

### What actually lands in Sprint 2

The sprint is not just the `TB-*` tasks. The following tasks from module sections below also land in Sprint 2 because the tracer-bullet slice depends on them being real, not stubbed:

- **M1-02, M1-03, M1-04** — `wildframe_ingest`. The ingest stage ships **real** in the slice (cheap to implement, no reason to stub). TB-09 asserts on its output directly.
- **M5-01** — `DeterministicMetadata` struct. TB-06's stub EXIF reader returns an instance of this type, so the struct must exist first.
- **M5-06 is folded into TB-07** — TB-07 registers the `wildframe_provenance:*` namespace with the shape M5-06 describes. When TB-07 merges, check M5-06 off in the same PR and cite "landed as part of TB-07" in the checkbox-flip commit message.
- **M6-01 is folded into TB-02** — TB-02 publishes the `PipelineStage` interface M6-01 describes. Close M6-01 the same way.
- **M7-11 becomes runnable in Sprint 2.** Once TB-02 publishes the orchestrator skeleton, [src/CMakeLists.txt](../src/CMakeLists.txt) can drop its direct links to `wildframe::ingest / raw / detect / focus` and get them transitively. Pick it up after TB-02 lands rather than waiting for M7-01.

### Suggested agent pickup order

Agents picking tasks sequentially should work this order. Parallelizable clusters are grouped:

1. **M1-02** (directory enumeration) and **TB-01** (CLI + TOML parser) — independent, can land in either order.
2. **M1-03, M1-04** — both depend on M1-02.
3. **M5-01** — depends only on S0-03 (done); can land in parallel with any of the above.
4. **TB-02** — depends on TB-01 + M1-01 (done).
5. **M7-11** — optional cleanup, runnable once TB-02 lands. Small.
6. **TB-03, TB-04, TB-05, TB-06** — all depend on TB-02; TB-06 additionally depends on M5-01. Independent stubs in different modules; fine to parallelize if multiple agents are active.
7. **TB-07, TB-08** — both depend on TB-02. Independent of each other.
8. **TB-09** — depends on every task above. Closes the sprint.

### TOML parser ownership

Per [docs/ARCHITECTURE.md §6](ARCHITECTURE.md) ("Every module → `tomlplusplus` config indirectly") the TOML parser lives in the **CLI/GUI entry point**, not in a module. Sprint 2's parser lives in `src/` alongside `main.cpp` and hands each downstream component a strongly-typed, pre-validated struct. The GUI entry point (Module 7) shares this parser when it lands.

### What does *not* land in Sprint 2

- No real ONNX Runtime link. TB-04 is a sentinel return; M3-01 introduces ONNX.
- No real OpenCV link on the focus path. TB-05 is a sentinel; M4-* introduces OpenCV.
- No `wildframe:*` AI-namespace or `wildframe_user:*` override XMP writes. Deferred to M5-05 / M5-07 / M5-08 / M5-10 to avoid committing sentinel floats to real XMP and polluting round-trip tests.
- No worker thread, no job queue, no cancellation. TB-02 is synchronous; M6-02 / M6-03 / M6-08 thicken the orchestrator later.
- No GUI. Module 7 builds on the already-thickened pipeline.

- [ ] **TB-01** — CLI entry point + TOML config parser
  - Deps: S0-03, S0-14, S0-18, S0-19
  - Size: M
  - Satisfies: §3 (delivery), FR-11
  - Replace the placeholder body of [src/main.cpp](../src/main.cpp) with a real CLI entry point.
  - **Arg parsing.** Hand-rolled loop over `argc`/`argv` — `--config <path>` (optional) and one positional `<directory>` (required). No new CLI-parsing dependency (NFR-6, no handoff-doc entry authorizing one). `--help` and `--version` are permissible, `-h`/`-v` shortcuts optional.
  - **Config parser.** Lives in `src/` alongside `main.cpp` per [docs/ARCHITECTURE.md §6](ARCHITECTURE.md) (TOML parsing is a GUI/CLI concern, not a module concern). Uses `tomlplusplus` (already in [vcpkg.json](../vcpkg.json)). Resolves the config path per [docs/CONFIG.md §1.2](CONFIG.md) (CLI arg → `$XDG_CONFIG_HOME/wildframe/config.toml` → macOS fallback → built-in default at [data/config.default.toml](../data/config.default.toml)). Implements the fail-loud error model in CONFIG.md §1.5, §1.6, §5.
  - **Per §2.11, parse only keys with live consumers in Sprint 2.** That is: `log_path`, `log_level`, `manifest_dir`, `reanalysis_default`. Keys under `[ingest]`, `[detect]`, `[focus]` are NOT parsed yet; they land with their owning thickening passes (M1-02, M3-*, M4-*). An unknown-key rejection (CONFIG.md §1.5) fires on the `[detect]` table today — that is intentional for the slice and the test fixture's config must not set those keys yet. M3-* and M4-* will each widen the parser in the same PR that adds their config consumer.
  - **Environment boundary (STYLE §2.12).** One function in `src/` reads `$HOME` / `$XDG_CONFIG_HOME`. Every other function takes resolved `std::filesystem::path` parameters. Tests drive the parser with synthetic paths, never by mutating the process environment.
  - **Entry-point shape.** Expose `int Run(int argc, const char* const* argv, std::ostream& err);` (or equivalent) in a header under `src/`. `main()` is a 3-line wrapper that calls `Run`; TB-09 calls `Run` directly rather than spawning a subprocess. This keeps the smoke test in-process and TSan/ASan-friendly.
  - **Wiring.** On successful parse: initialize `wildframe::log` via `Init()` (S0-14) with the parsed `log_path` / `log_level`, hand the orchestrator (TB-02, lands next) its constructed stage list and the resolved `manifest_dir`, run the batch synchronously, return `0` on full success and a non-zero status distinguishing "configuration error" from "partial batch failure" (the latter is the M6-04 error-isolation surface; until M6-04 lands, any stage exception aborts the run).
  - **Not in scope for this task.** No job queue, no worker thread, no cancellation — those are M6-02 / M6-03 / M6-08. No `reanalysis_default` handling yet — M7-10 consumes it; TB-01 only validates the value.
  - CLI persists alongside the GUI once Module 7 lands. It is the headless / automation entry point already scoped by FR-10 and FR-11.

- [ ] **TB-02** — Orchestrator skeleton (sequential)
  - Deps: TB-01, M1-01
  - Size: M
  - Satisfies: NFR-3 (stage-registration extension point), §12 (data flow)
  - **Folds M6-01** — when TB-02 merges, flip M6-01's checkbox in the same PR with the commit note "landed in TB-02".
  - Publishes `libs/orchestrator/include/wildframe/orchestrator/pipeline_stage.hpp` with an abstract `PipelineStage` base: `virtual StageResult Process(StageContext&) = 0;` plus a `virtual ~PipelineStage() = default;`. `StageContext` carries the current `ImageJob` plus whatever the stage has produced so far (preview buffer, EXIF, detection, focus) — exact shape is this task's design call, but must be extensible without ABI thrash (prefer a struct of `std::optional<...>` members, not a variant).
  - Publishes `libs/orchestrator/include/wildframe/orchestrator/orchestrator.hpp` additions: a class that accepts a `std::vector<std::unique_ptr<PipelineStage>>` (ownership transferred in), the resolved `manifest_dir`, and exposes `RunResult Run(std::span<const ingest::ImageJob> jobs)`. The MVP implementation is a plain `for`-loop over jobs on the calling thread — **no threading, no queue**. Those are M6-02 / M6-03 / M6-08, which may freely refactor this skeleton.
  - Progress callback hook (`std::function<void(ProgressUpdate)>`) is accepted by the constructor and may be a no-op until M6-06 wires it.
  - Links `src/` against `wildframe::orchestrator` (+ `wildframe::log` + `wildframe::metadata`); the direct links to ingest/raw/detect/focus in [src/CMakeLists.txt](../src/CMakeLists.txt) become transitive. M7-11 closes this cleanup.
  - Error isolation is **not** wired yet — a stage exception propagates out of `Run` and aborts the CLI. M6-04 catches them and writes manifest rows.

- [ ] **TB-03** — Stub `wildframe_raw` preview/decode
  - Deps: TB-02
  - Size: S
  - **Introduces** the public `PreviewImage` type in `libs/raw/include/wildframe/raw/preview_image.hpp` — the output type of every raw-decode operation and the input type of `wildframe_detect` / `wildframe_focus`. Shape: `{int width, int height, std::vector<std::uint8_t> rgb_bytes}` or similar. Value type, Rule of Zero (STYLE §2.5). `BBox` — used by `decode_crop` — lives in `wildframe_detect`'s public header per TB-04 (it is produced by detect, consumed by raw and focus).
  - **Introduces** `PreviewImage ExtractPreview(const std::filesystem::path&)` and `PreviewImage DecodeCrop(const std::filesystem::path&, const BBox&)` declarations in `libs/raw/include/wildframe/raw/raw.hpp` — the signatures M2-01 / M2-02 will satisfy unchanged.
  - Stub implementation returns a deterministic synthetic 256×256 RGB buffer filled with mid-gray (128,128,128). `DecodeCrop` returns a buffer sized to the requested bbox, same gray fill. No LibRaw link yet.
  - Publishes a `RawStage : PipelineStage` in `libs/raw/src/raw_stage.cpp` that calls `ExtractPreview` and writes the result into `StageContext`. TB-02's orchestrator receives an instance of this stage.
  - Tests (`libs/raw/tests/raw_test.cpp`): buffer dimensions, determinism across calls, non-empty-for-any-fixture-path. Replaced by M2-01 / M2-02.

- [ ] **TB-04** — Stub `wildframe_detect` stage
  - Deps: TB-02, TB-03 (for `PreviewImage`)
  - Size: S
  - **Introduces** the public `BBox` type in `libs/detect/include/wildframe/detect/bbox.hpp` — `{float x, float y, float width, float height}` with a member `float Area() const noexcept`. Re-used by `wildframe_raw::DecodeCrop` and `wildframe_focus::Score`.
  - **Introduces** the public `DetectionResult` and `DetectConfig` types in `libs/detect/include/wildframe/detect/detect.hpp`. `DetectionResult` fields per handoff §13. `DetectConfig` is empty for now; the real fields (`confidence_threshold`, `iou_threshold`, `model`, `execution_provider`) land with M3-01 / M3-03 / M3-06 in the same PR that parses those TOML keys.
  - **Introduces** `DetectionResult Detect(const raw::PreviewImage&, const DetectConfig&)` declaration — the signature M3-05 will satisfy unchanged.
  - Stub returns `DetectionResult{bird_present=false, bird_count=0, bird_boxes={}, primary_subject_box=std::nullopt, detection_confidence=0.0F}` regardless of input. No ONNX Runtime link — the `wildframe_detect` CMake target does not depend on `onnxruntime` until M3-01 wires it.
  - Publishes a `DetectStage : PipelineStage` in `libs/detect/src/detect_stage.cpp`.
  - Tests pin the sentinel return. Replaced by M3-01..M3-05.

- [ ] **TB-05** — Stub `wildframe_focus` stage
  - Deps: TB-02, TB-03 (for `PreviewImage`), TB-04 (for `BBox`)
  - Size: S
  - **Introduces** the public `FocusResult` and `FocusConfig` types in `libs/focus/include/wildframe/focus/focus.hpp`. `FocusResult`: `{float focus_score, float motion_blur_score, float subject_size_percent, float keeper_score, std::array<bool, 4> edge_clipped}` per handoff §13 and docs/METADATA.md §3.2. `FocusConfig` is empty for now; real fields (`laplacian_saturation`, keeper weights, edge clipping tolerance) land with M4-02 / M4-04 / M4-05 in the same PR that parses those TOML keys.
  - **Introduces** `FocusResult Score(const raw::PreviewImage&, const std::optional<detect::BBox>&, const FocusConfig&)` — the signature M4-06 will satisfy unchanged.
  - Stub returns `FocusResult{focus_score=0.0F, motion_blur_score=0.0F, subject_size_percent=0.0F, keeper_score=0.0F, edge_clipped={false,false,false,false}}` regardless of input. No OpenCV link — the `wildframe_focus` CMake target does not depend on `opencv4` until M4-01 wires it.
  - Publishes a `FocusStage : PipelineStage`. Tests pin the sentinel return. Replaced by M4-01..M4-07.

- [ ] **TB-06** — Stub `wildframe_metadata` EXIF reader
  - Deps: TB-02, M5-01
  - Size: S
  - **Introduces** `metadata::DeterministicMetadata ReadExif(const std::filesystem::path&)` declaration in `libs/metadata/include/wildframe/metadata/metadata.hpp` — the signature M5-02 will satisfy unchanged.
  - Stub returns a `DeterministicMetadata` with every `std::optional<T>` field as `std::nullopt` regardless of input. No Exiv2 read-side link yet (TB-07 separately introduces the Exiv2 *write* link; both sides converge when M5-02 thickens the read path).
  - Publishes a `MetadataReadStage : PipelineStage`. Tests pin that every field is `nullopt`. Replaced by M5-02..M5-04.

- [ ] **TB-07** — Provenance-only XMP sidecar writer
  - Deps: TB-02
  - Size: M
  - Satisfies: FR-5 (partial), NFR-5
  - **Folds M5-06** — when TB-07 merges, flip M5-06's checkbox in the same PR with the commit note "landed as part of TB-07".
  - Links `wildframe_metadata` against Exiv2 for the first time. Registers the `wildframe_provenance:*` namespace per handoff §13 + [docs/METADATA.md §4](METADATA.md). Writes `<raw>.xmp` via Exiv2's high-level API using the temp-file-plus-rename idiom (M5-08 will reuse this machinery unchanged).
  - Populated provenance fields on every sidecar: `analysis_timestamp` (ISO-8601, local), `pipeline_version` (from a single compile-time constant — add `libs/metadata/include/wildframe/metadata/version.hpp` with `constexpr std::string_view kPipelineVersion = "0.0.0";`), `detector_model_name = "stub"`, `detector_model_version = "0.0.0"`, `focus_algorithm_version = "0.0.0"`.
  - **`wildframe:*` AI-namespace and `wildframe_user:*` override namespace are deliberately deferred** to M5-05 / M5-07 / M5-08 / M5-10. The AI namespace would only carry sentinels anyway, and writing sentinels as real XMP pollutes the round-trip test surface for the thickening passes.
  - Publishes a `MetadataWriteStage : PipelineStage` in `libs/metadata/src/metadata_write_stage.cpp`. The stage pulls whatever is in `StageContext` (stub DetectionResult, stub FocusResult) and writes **only** provenance fields, ignoring the AI data for now. M5-08 rewires the stage to emit AI fields once the AI namespace exists.
  - Tests: write a sidecar for a fixture CR3, reopen it with Exiv2, verify the five provenance fields are present and correctly typed. Temp-file-plus-rename atomicity test: kill a write mid-flight (simulate via an injected throwing sink or by crashing on a callback) and confirm the original file is intact.

- [ ] **TB-08** — Minimal batch manifest writer
  - Deps: TB-02, S0-19
  - Size: S
  - Satisfies: FR-5 (partial), NFR-5
  - Publishes a `BatchManifestWriter` type in `libs/orchestrator/` (same module as `paths.hpp` — manifest is an orchestrator responsibility per [docs/ARCHITECTURE.md §3](ARCHITECTURE.md)). Constructs from the resolved `manifest_dir` (`orchestrator::DefaultManifestDir(home)` unless overridden). On open, creates the directory if missing and opens `<manifest_dir>/<ISO-8601-timestamp>.json` for writing.
  - Shape (nlohmann/json, pretty-printed for human inspection during MVP):
    ```json
    { "pipeline_version": "0.0.0",
      "started_at":  "<ISO-8601>",
      "finished_at": "<ISO-8601>",
      "status": "completed" | "partial",
      "images": [ { "input_path": "...", "sidecar_path": "...",
                    "stage_timings_ms": { "ingest": ..., "raw": ..., ... },
                    "error": null } ] }
    ```
  - `status` is `completed` when every job returns without exception, `partial` otherwise. `cancelled` status lands with M6-08. Per-image `error` stays `null` for Sprint 2 — per-image error isolation is M6-04; until then, a stage exception aborts the run and `BatchManifestWriter` finishes the current partial file in its destructor.
  - The orchestrator (TB-02) wires this: constructs the writer before `Run`, appends a row per job completion, calls `Finalize(status)` before returning. M6-05 extends the shape (more fields, richer timings, error rows) rather than rewriting.
  - Tests: write a manifest over a synthetic job list, reopen with nlohmann/json, assert the row count and timestamp shape. Assert the file survives a destructor-during-write path.

- [ ] **TB-09** — End-to-end CLI smoke test
  - Deps: TB-01, TB-02, TB-03, TB-04, TB-05, TB-06, TB-07, TB-08, M1-02, M1-03, M1-04, S0-12
  - Size: S
  - Satisfies: NFR-6
  - GoogleTest integration test living at [tests/e2e_cli_test.cpp](../tests/) (the top-level `tests/` directory exists but is empty — this is its first inhabitant, so also add `tests/CMakeLists.txt` and wire it into the root build). Uses a `gtest` target named `wildframe_e2e_test`.
  - Calls `Run(argc, argv, err)` from TB-01 directly — **not** a subprocess spawn. Keeps the test in-process, TSan/ASan-friendly, and fast.
  - Per test case: copy the S0-12 fixture tree into a `testing::TempDir()` (fixtures must not be mutated in place — XMP sidecars are written next to the RAW), point the CLI at that copy, pass a synthetic config that overrides `log_path` and `manifest_dir` into the temp dir too so the test does not touch `~/Library/Logs/`.
  - Assertions:
    - `Run` returns `0`.
    - One `*.xmp` sidecar next to every fixture CR3.
    - Each sidecar opens with Exiv2 and contains the five provenance fields from TB-07.
    - Exactly one manifest file exists under the temp `manifest_dir`.
    - The manifest parses as JSON, has `status: "completed"`, has one `images[]` row per fixture, and each row's `sidecar_path` exists.
  - **Re-run as each thickening pass lands.** Any regression blocks that pass's merge. As stubs are replaced, extend this test with per-thickening assertions (e.g., after M3-* lands, assert `bird_present` is true on the "clear bird" fixture) rather than creating a new smoke test.
  - Supersedes the original plan for `I-01` as a CLI-side end-to-end test; `I-01` is re-scoped to GUI-coupled end-to-end validation.

---

## Module 1 — `wildframe_ingest` (FR-1)

- [x] **M1-01** — `ImageJob` public struct
  - Deps: S0-03
  - Size: S
  - Public header: `libs/ingest/include/wildframe/ingest/image_job.hpp`. Fields: absolute path, format tag (enum, `Cr3` only in MVP), file size, content hash (optional, deferred). Value type, copyable, immutable.

- [ ] **M1-02** — Directory enumeration with CR3 filter
  - Deps: M1-01
  - Size: M
  - Satisfies: FR-1
  - **Lands in Sprint 2** — `wildframe_ingest` ships real in the tracer-bullet slice; TB-09 consumes this output directly.
  - `std::vector<ImageJob> Enumerate(const std::filesystem::path& dir, int max_depth = 1)`. Skips symlinks by default. Returns sorted by path for determinism. Consumes the `[ingest].max_depth` TOML key — widens TB-01's parser with that key in the same PR.

- [ ] **M1-03** — CR3 file validation
  - Deps: M1-02
  - Size: S
  - Satisfies: FR-1
  - **Lands in Sprint 2** — TB-09 requires the fixture tree's CR3s to validate and non-CR3s to be skipped.
  - Validate by extension + magic bytes. Malformed files are logged and skipped, not aborted.

- [ ] **M1-04** — Error translation at boundary
  - Deps: M1-02
  - Size: S
  - Satisfies: NFR-7 (exception policy)
  - **Lands in Sprint 2** — TB-09 expects the CLI not to crash on a bad fixture path; the orchestrator catches `IngestError` at the module boundary per STYLE §3.1.
  - Filesystem exceptions caught, translated to `wildframe::ingest::IngestError`.

- [ ] **M1-05** — Unit tests for ingest
  - Deps: M1-03, M1-04, S0-12
  - Size: M
  - Satisfies: NFR-6 (coverage)
  - GoogleTest target `wildframe_ingest_test`. Covers: empty dir, dir with CR3 + non-CR3 files, nested dir at depth 0 and 1, unreadable file, invalid path.

---

## Module 2 — `wildframe_raw` (FR-2)

Thickening pass — replaces the **TB-03** stub. Each task below lands without regressing TB-09.

- [ ] **M2-01** — LibRaw preview extraction API
  - Deps: S0-03
  - Size: M
  - Satisfies: FR-2
  - `PreviewImage ExtractPreview(const std::filesystem::path& raw_path)`. Returns largest embedded JPEG as decoded RGB buffer + dimensions. RAII wrapper around `LibRaw` handle.

- [ ] **M2-02** — Full-decode crop API (fallback)
  - Deps: M2-01
  - Size: M
  - Satisfies: FR-2, FR-4
  - `CroppedImage DecodeCrop(const std::filesystem::path& raw_path, BBox crop_region)`. Used only when preview resolution is insufficient for focus scoring.

- [ ] **M2-03** — Exception translation at boundary
  - Deps: M2-01
  - Size: S
  - Satisfies: NFR-7
  - `LibRaw` errors caught and translated to `wildframe::raw::RawDecodeError`.

- [ ] **M2-04** — Unit tests for raw
  - Deps: M2-02, M2-03, S0-12
  - Size: M
  - Satisfies: NFR-6
  - GoogleTest target. Covers: preview extraction from all 5 fixture categories, decode_crop at multiple regions, invalid file error path.

---

## Module 5a — `wildframe_metadata` (EXIF read portion of FR-5)

Thickening pass — replaces the **TB-06** stub (M5-02..M5-04). M5-01 lands before TB-06 so the stub can return the real struct type.

- [x] **M5-01** — `DeterministicMetadata` struct
  - Deps: S0-03
  - Size: S
  - **Lands in Sprint 2** — TB-06's stub EXIF reader returns a `DeterministicMetadata` instance, so the struct must exist first.
  - Public header: `libs/metadata/include/wildframe/metadata/deterministic_metadata.hpp`. All fields from handoff §13 deterministic-metadata list. Every field is `std::optional<T>` — missing EXIF tags stay `nullopt` rather than forcing a sentinel value. Value type, Rule of Zero.

- [ ] **M5-02** — Exiv2 EXIF reader
  - Deps: M5-01
  - Size: M
  - Satisfies: FR-5
  - `DeterministicMetadata ReadExif(const std::filesystem::path& raw_path)`. Maps Exiv2 keys to struct fields per the schema. Missing fields become `std::optional::nullopt`.

- [ ] **M5-03** — Exception translation at boundary
  - Deps: M5-02
  - Size: S
  - Satisfies: NFR-7
  - Exiv2 errors caught and translated to `wildframe::metadata::MetadataError`.

- [ ] **M5-04** — Unit tests for EXIF read
  - Deps: M5-03, S0-12
  - Size: M
  - Satisfies: NFR-6
  - Covers all fixture categories. Verifies camera_model, lens_model, focal_length, etc. round-trip correctly.

---

## Module 3 — `wildframe_detect` (FR-3)

Thickening pass — replaces the **TB-04** stub. Introduces the `onnxruntime` link for the first time. Each task lands without regressing TB-09.

- [ ] **M3-01** — ONNX Runtime session wrapper
  - Deps: S0-03, S0-11
  - Size: M
  - Satisfies: FR-3, §5 (GPU strategy)
  - RAII wrapper around `Ort::Session`. Configurable EP: CPU or CoreML. Selected via runtime config, default CoreML with CPU fallback.

- [ ] **M3-02** — YOLOv11 preprocessing
  - Deps: M3-01
  - Size: M
  - Letterbox resize to model input size (typically 640×640), normalize to [0,1], transpose HWC→CHW, float32 tensor.

- [ ] **M3-03** — YOLOv11 postprocessing
  - Deps: M3-02
  - Size: M
  - Decode detection head output. Apply confidence threshold (configurable, default 0.25). Apply non-max suppression (IoU threshold configurable, default 0.45). Filter to "bird" class (COCO class 14).

- [ ] **M3-04** — `DetectionResult` struct and primary-subject selection
  - Deps: M3-03
  - Size: S
  - Satisfies: FR-3
  - Struct: `bird_present`, `bird_count`, `bird_boxes`, `primary_subject_box`, `detection_confidence`. Primary subject selected by max confidence, ties broken by bbox area.

- [ ] **M3-05** — Top-level `detect` API
  - Deps: M3-04
  - Size: S
  - `DetectionResult Detect(const PreviewImage& preview, const DetectConfig& cfg)`.

- [ ] **M3-06** — MegaDetector alternative path
  - Deps: M3-05
  - Size: M
  - Satisfies: §15 (mitigation for false positives)
  - Config-selectable. Same `DetectionResult` output contract. Separate preprocessing/postprocessing but shared session wrapper.

- [ ] **M3-07** — Unit tests for detect
  - Deps: M3-05, M3-06, S0-12
  - Size: M
  - Satisfies: NFR-6
  - Covers: all 5 fixture categories for both models. Assertions on expected bird_present and rough bbox sanity checks, not exact coordinates.

---

## Module 4 — `wildframe_focus` (FR-4)

Thickening pass — replaces the **TB-05** stub. Introduces the `opencv` link on the focus side for the first time. Each task lands without regressing TB-09.

- [ ] **M4-01** — Bird crop extraction utility
  - Deps: S0-03
  - Size: S
  - Given a preview image and a bbox, return the cropped grayscale region for scoring.

- [ ] **M4-02** — Variance-of-Laplacian focus score
  - Deps: M4-01
  - Size: S
  - Satisfies: FR-4
  - OpenCV `cv::Laplacian` + variance. Normalized to [0,1] via a documented saturating mapping; mapping parameters configurable.

- [ ] **M4-03** — FFT motion-blur score
  - Deps: M4-01
  - Size: M
  - Satisfies: FR-4
  - Ratio of high-frequency energy to total energy in the FFT magnitude spectrum. Normalized to [0,1].

- [ ] **M4-04** — Subject size and edge-clipping metrics
  - Deps: M4-01
  - Size: S
  - Satisfies: FR-4
  - `subject_size_percent` = bbox area / image area. Edge-clipping: boolean per bbox edge that touches the image boundary within a small tolerance.

- [ ] **M4-05** — Keeper score combination formula
  - Deps: M4-02, M4-03, M4-04
  - Size: M
  - Satisfies: FR-4
  - Weighted sum of focus_score, motion_blur_score, subject_size, with edge-clipping penalty. Weights live in a versioned `FocusConfig` struct loaded from a runtime config file. The formula is documented in `docs/METADATA.md`.

- [ ] **M4-06** — Top-level `score` API
  - Deps: M4-05
  - Size: S
  - `FocusResult Score(const PreviewImage& preview, const BBox& primary_subject, const FocusConfig& cfg)`.

- [ ] **M4-07** — Unit tests for focus
  - Deps: M4-06, S0-12
  - Size: M
  - Satisfies: NFR-6, NFR-2
  - Fixture-based. Must show: sharp fixture scores higher than motion-blurred fixture, no-bird fixture has keeper_score close to zero when no bbox is provided.

---

## Module 5b — `wildframe_metadata` (XMP write portion of FR-5 + FR-6)

Thickening pass — **extends** the **TB-07** provenance-only writer with the `wildframe:*` AI namespace (M5-05, M5-08) and the `wildframe_user:*` override namespace (M5-07, M5-10), plus sidecar read (M5-09) and round-trip coverage (M5-11). TB-07's provenance-namespace registration (M5-06 shape) and temp-file-plus-rename idiom (M5-08 shape) land in TB-07 and are reused here — these tasks extend rather than rewrite.

- [ ] **M5-05** — `wildframe:*` XMP namespace registration
  - Deps: M5-03
  - Size: S
  - Satisfies: FR-5, NFR-4
  - Register namespace and field definitions with Exiv2 at startup. Mirror `docs/METADATA.md` exactly.

- [ ] **M5-06** — `wildframe_provenance:*` namespace
  - Deps: M5-05
  - Size: S
  - Satisfies: NFR-5
  - **Folded into TB-07.** TB-07 registers this namespace and its fields per handoff §13 / [docs/METADATA.md §4](METADATA.md). Flip this checkbox in the TB-07 PR with the commit note "landed as part of TB-07". Preserved as a task ID only for traceability.

- [ ] **M5-07** — `wildframe_user:*` namespace
  - Deps: M5-05
  - Size: S
  - Satisfies: FR-6
  - Fields per handoff §13.

- [ ] **M5-08** — XMP sidecar writer
  - Deps: M5-05, M5-06
  - Size: M
  - Satisfies: FR-5
  - `void WriteSidecar(const std::filesystem::path& raw_path, const AiMetadata& ai, const ProvenanceMetadata& prov)`. Uses Exiv2 high-level API only. Atomic write via temp-file + rename. Preserves existing non-wildframe XMP content.

- [ ] **M5-09** — XMP sidecar reader
  - Deps: M5-05, M5-06, M5-07
  - Size: M
  - Satisfies: FR-6, FR-8
  - Returns all wildframe namespaces + user overrides. Used by GUI for filtering and the detail view.

- [ ] **M5-10** — User override write API
  - Deps: M5-07, M5-08
  - Size: S
  - Satisfies: FR-6
  - `void WriteUserOverride(const std::filesystem::path& raw_path, const UserOverride& override)`. Does not modify the AI namespace.

- [ ] **M5-11** — Round-trip tests
  - Deps: M5-08, M5-09, M5-10
  - Size: M
  - Satisfies: NFR-5
  - Write AI metadata → read back → verify equality. Same for overrides. Verify that existing non-wildframe XMP keys survive a write.

---

## Module 6 — `wildframe_orchestrator` (wires everything)

Thickening pass — **extends** the **TB-02** sequential skeleton with the job queue (M6-02), the worker thread + full sequential pipeline execution (M6-03), per-image error isolation (M6-04), the full batch manifest (M6-05 extends TB-08), the progress callback (M6-06), and cooperative cancellation (M6-08). M6-01's `PipelineStage` interface publishes in TB-02. Each task lands without regressing TB-09.

- [ ] **M6-01** — `PipelineStage` interface for extensibility
  - Deps: S0-03
  - Size: M
  - Satisfies: NFR-3
  - **Folded into TB-02.** TB-02 publishes `PipelineStage` and the concrete stage classes (`RawStage`, `DetectStage`, `FocusStage`, `MetadataReadStage`, `MetadataWriteStage`) land with the respective TB-03..TB-07 stubs. Phase 2+ can add stages without touching the orchestrator core. Flip this checkbox in the TB-02 PR with the commit note "landed in TB-02". Preserved as a task ID only for traceability.

- [ ] **M6-02** — Job queue
  - Deps: M6-01
  - Size: M
  - Thread-safe FIFO. MVP is single-consumer, but the interface supports multi-consumer for Phase 2+.

- [ ] **M6-03** — Worker-thread pipeline execution
  - Deps: TB-02, M6-02
  - Size: M
  - Satisfies: §12 (data flow), NFR-9 (thread safety)
  - Upgrades TB-02's synchronous `Run` to a background worker thread that drains the M6-02 job queue. Per-job stage sequence (ingest → raw → metadata EXIF read → detect → focus → metadata XMP write) is unchanged — TB-02 already wired it — this task just moves the loop off the caller's thread. Publishes the thread-safety contract in `libs/orchestrator/include/wildframe/orchestrator/orchestrator.hpp` per NFR-9.

- [ ] **M6-04** — Per-image error isolation
  - Deps: M6-03
  - Size: S
  - Satisfies: FR-5 (error handling), NFR-6
  - One bad file logs an error row to the batch manifest and continues. Only catastrophic errors (out-of-memory, corrupt ONNX model) abort the batch.

- [ ] **M6-05** — Batch JSON manifest writer
  - Deps: M6-03
  - Size: M
  - Satisfies: FR-5, NFR-5
  - Written to a run-scoped folder (path convention TBD in S0-09 or a config decision). Append-only during the run. Includes per-image input path, output sidecar path, timings per stage, error (if any), and pipeline/provenance metadata once at the top.

- [ ] **M6-06** — Progress reporting callback
  - Deps: M6-03
  - Size: S
  - `std::function<void(ProgressUpdate)>` invoked per job completion. Used by the GUI.

- [ ] **M6-07** — Orchestrator-specific integration tests
  - Deps: M6-03, M6-04, M6-05, M6-06, M6-08
  - Size: M
  - Satisfies: NFR-6
  - End-to-end smoke over the fixture set is already covered by TB-09. This task adds the orchestrator-specific behavior TB-09 does not: per-image error isolation (inject a failing stage, assert other jobs continue and the manifest row carries the error), cooperative cancellation (cancel mid-batch, assert partial results are retained and manifest status is `cancelled`), worker-thread safety (assert `Run` returns before the worker finishes and progress callbacks marshal correctly), and the full manifest shape M6-05 exposes.

- [ ] **M6-08** — Cooperative batch cancellation
  - Deps: M6-03
  - Size: M
  - Satisfies: FR-9
  - Orchestrator exposes a `Cancel()` method. When set, the worker finishes the current per-image step, writes any partial output for that image if stage boundaries allow, stops dispatching new work, records `status: cancelled` in the manifest, and returns. No forceful thread termination.

---

## Module 7 — `wildframe_gui` (FR-6)

- [ ] **M7-01** — Main window + directory picker
  - Deps: S0-03, M6-06
  - Size: M
  - Satisfies: FR-6
  - Qt 6 Widgets. Menu: File → Open Directory. On select, enumerates and displays file count.

- [ ] **M7-02** — Batch runner (invokes orchestrator on worker thread)
  - Deps: M7-01, M6-07
  - Size: M
  - Satisfies: FR-6, §5 (threading strategy)
  - GUI submits a batch to the orchestrator on a dedicated worker thread. Progress updates marshalled back to the main thread via Qt signals.

- [ ] **M7-03** — Progress view
  - Deps: M7-02
  - Size: S
  - Progress bar, current file, elapsed time, per-stage timing rollup.

- [ ] **M7-04** — Thumbnail grid
  - Deps: M7-02, M5-09
  - Size: L
  - Satisfies: FR-6
  - Loads previews + sidecar metadata as results arrive. Draws keeper-score indicator overlay on each thumbnail.

- [ ] **M7-05** — Detail view
  - Deps: M7-04
  - Size: M
  - Satisfies: FR-6
  - Clicking a thumbnail shows preview with drawn bbox, all scores, EXIF summary.

- [ ] **M7-06** — Filter controls
  - Deps: M7-04
  - Size: M
  - Satisfies: FR-6, FR-8
  - Keeper-score range slider, bird-present toggle, detection-confidence range slider. Filter applies to the visible grid.

- [ ] **M7-07** — Override UI
  - Deps: M7-05, M5-10
  - Size: M
  - Satisfies: FR-6
  - Approve/reject toggle, bird-present override toggle, free-text note field. Changes written back to XMP sidecar via `wildframe_metadata`.

- [ ] **M7-08** — GUI logic tests (non-widget)
  - Deps: M7-04, M7-06, M7-07
  - Size: M
  - Satisfies: NFR-6
  - GoogleTest covers: filter predicates, thumbnail model population, override state machine. Widget rendering itself is not unit-tested in MVP.

- [ ] **M7-09** — Cancel button and progress wiring
  - Deps: M7-03, M6-08
  - Size: S
  - Satisfies: FR-9
  - Cancel button in the progress view invokes the orchestrator's `Cancel()`. UI reflects `cancelling…` state until the worker confirms completion, then shows partial-results summary.

- [ ] **M7-10** — Re-analysis prompt dialog
  - Deps: M7-01, M5-09, S0-21
  - Size: M
  - Satisfies: FR-10
  - On batch start, scan for existing `wildframe:*` sidecars. If any exist, show modal with three options (skip already-analyzed, overwrite all, cancel) and a count of how many files are affected. Per-file override is exposed later from the detail view (part of M7-07). Decision recorded to manifest and `wildframe_user:reanalysis_policy_used`.

- [ ] **M7-11** — Trim CLI / GUI link line to architectural contract
  - Deps: TB-02
  - Size: S
  - Satisfies: NFR-3, NFR-6
  - **Runnable in Sprint 2** once TB-02 publishes the orchestrator skeleton.
  - [src/CMakeLists.txt](../src/CMakeLists.txt) currently links the stub `wildframe` executable against all six static libraries. [docs/ARCHITECTURE.md §1](ARCHITECTURE.md) specifies the entry point depends only on `wildframe::orchestrator` and `wildframe::metadata` (plus `wildframe::log`); the other four are indirect. After TB-02 lands, remove the direct links to `wildframe::ingest`, `wildframe::raw`, `wildframe::detect`, `wildframe::focus` so the module boundary is enforced by the build, not just documented. Identified by the 2026-04-21 Sprint 0 infrastructure review §5.

---

## Integration / Validation

- [ ] **I-01** — GUI end-to-end smoke test
  - Deps: M6-07, M7-08
  - Size: S
  - Satisfies: §16 (MVP readiness)
  - GUI-side counterpart to TB-09. Run the GUI on all S0-12 fixtures. Verify sidecars, manifest, filterability, and that GUI overrides round-trip through `wildframe_metadata`. The CLI-side end-to-end smoke is TB-09 and must remain green alongside.

- [ ] **I-02** — Performance benchmark on Intel Mac
  - Deps: I-01
  - Size: M
  - Satisfies: NFR-1
  - 500-image synthetic batch (can reuse fixtures). Measure end-to-end wall time. Target: <10 minutes. Report per-stage breakdown.

- [ ] **I-03** — Performance benchmark on Apple Silicon (deferred until hardware)
  - Deps: I-02
  - Size: M
  - Satisfies: NFR-1
  - Target: <2 minutes on same 500-image batch with CoreML EP.

- [ ] **I-04** — XMP compatibility check with Lightroom
  - Deps: I-01
  - Size: S
  - Satisfies: FR-8, NFR-4
  - Import a Wildframe-analyzed batch into Lightroom Classic. Confirm custom fields are readable and smart collections can filter on them.

- [ ] **I-05** — User acceptance test
  - Deps: I-04
  - Size: M
  - Customer (photographer) runs Wildframe on a real shoot. Collect feedback, triage into Phase 2 backlog.

---

## Post-MVP — Planning

Tasks here are **not** part of MVP and must not be picked up until `I-05` is signed off. They exist to capture scope-expansion questions the customer wants parked, not commitments.

- [ ] **P2-01** — Scope cross-compilation: platforms, hardware, execution providers
  - Deps: I-05
  - Size: M (planning, not implementation)
  - Satisfies: §3 (platform), §15 (risks)
  - Deliverable: a decision record (plan-change PR per CONTRIBUTING.md if the answer changes the handoff) covering:
    - Which host platforms to support beyond macOS (Linux-on-x86_64, Linux-on-ARM, Windows-on-x86_64, Windows-on-ARM) and why.
    - Target hardware profiles per platform: CPU arch, minimum RAM, GPU/NPU availability (NVIDIA CUDA, AMD ROCm, Intel NPU, Windows DirectML, etc.).
    - Execution-provider choice per target — CoreML is Apple-only, so `wildframe_detect` (M3) needs a per-platform EP decision. Implications for the `DetectConfig` schema and the primary NFR-1 perf target.
    - GUI strategy per target — Qt 6 Widgets works on Linux/Windows, but packaging, signing, and dynamic-linking obligations (LGPL per docs/LICENSING.md) differ.
    - Build-infra impact: CI matrix, vcpkg triplets, fixture availability per platform, `tools/fetch_models.cmake` mirroring.
    - Concrete follow-up tasks (populate `cmake/platforms/linux.cmake` and `cmake/platforms/windows.cmake` per S0-20, revisit `cmake/ClangTooling.cmake` Homebrew-isms, CI jobs, triplet selection) with sizing so they can land in a sprint.
  - **Agent directive: do not start before I-05 is signed off.** This is a scoping task, not implementation; picking it up early is the exact scope creep CLAUDE.md §5 forbids.

- [ ] **P3-01** — Scope hosted-inference SaaS commercialization path
  - Deps: Phase 2 species-ID module shipped and in customer hands; not tracked yet.
  - Size: M (planning, not implementation)
  - Satisfies: n/a to the current handoff — this task proposes a handoff amendment, because nothing in `bird_photo_ai_project_handoff.md` currently scopes commercialization or network I/O.
  - Context: once species ID is in users' hands, the natural next lever is running detection / species ID / future components on Wildframe-hosted infrastructure, sold as metered API credits. The swap is feasible because module boundaries already isolate inference behind neutral interfaces (`wildframe_detect`, `wildframe_focus`, Phase 2 species-ID). This task decides whether to pursue it and what the shape is; no code lands under this ID.
  - Deliverable: a decision record (plan-change PR per CONTRIBUTING.md, since this materially changes the handoff's scope boundaries in §4 and §6) covering:
    - Whether to offer a paid subscription where inference components run on hosted infrastructure and clients purchase API credits.
    - **Substitutability audit of current interfaces.** Confirm `wildframe_detect` (M3), `wildframe_focus` (M4), and the Phase 2 species-ID module expose types that traffic in neutral data (image buffers in, result structs out) and do not leak ONNX Runtime, OpenCV, or LibRaw symbols across their public API. If they do, file remediation tasks **before** any wire protocol is designed; retrofitting an abstraction under a shipped interface is painful.
    - **Distribution shape.** Desktop + remote inference vs. Lightroom plugin (currently out of MVP per handoff §6) vs. web-based batch uploader. Once server-side inference is the value prop, the Lightroom-plugin branch becomes a much stronger distribution story — revisit §6 deliberately rather than by default.
    - **What stays local vs. what goes over the wire.** RAW decode and XMP sidecar write must remain client-side (bandwidth, user file ownership, offline safety). Uploads should be decoded previews or crops, not CR3s.
    - **Wire protocol.** Versioned from day one. Authentication (OIDC + QKeychain or platform equivalent), token refresh, what happens when the server rev breaks the client contract, graceful degradation to offline mode if credits exhausted mid-batch.
    - **Commercial surface area.** Account management, metered billing, credit-exhausted-mid-batch UX, auto-update infrastructure (Sparkle / Qt Installer Framework), telemetry posture consistent with NFR-5, cross-platform pressure this creates (Windows becomes a revenue question, not a nice-to-have — see P2-01).
    - **IP and licensing impact.** Subcontracted closed-source models, terms of service, data-retention commitments, and the interaction with the GPL-3.0-or-later client license recorded in `docs/LICENSING.md §1.1`. Server components and network-service obligations likely need separate licensing analysis (AGPL vs. GPL vs. proprietary server) — not a decision to make under this ID, but one to surface.
    - Concrete follow-up tasks with sizing so the implementation work can land in subsequent sprints.
  - **Agent directive: do not start before Phase 2 species-ID is shipped to users AND the customer explicitly signals readiness by ID.** This is a scoping task, not implementation; picking it up early is the exact scope creep CLAUDE.md §5 forbids. Any implementation work arising from this decision record must land under new task IDs after the plan-change PR merges.

---

## Conventions for adding tasks

- New tasks are appended at the end of the relevant section.
- IDs are never reused or renumbered.
- Removed tasks are struck through with a one-line reason, not deleted, so history stays traceable.
