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

## Module 1 — `wildframe_ingest` (FR-1)

- [x] **M1-01** — `ImageJob` public struct
  - Deps: S0-03
  - Size: S
  - Public header: `libs/ingest/include/wildframe/ingest/image_job.hpp`. Fields: absolute path, format tag (enum, `Cr3` only in MVP), file size, content hash (optional, deferred). Value type, copyable, immutable.

- [ ] **M1-02** — Directory enumeration with CR3 filter
  - Deps: M1-01
  - Size: M
  - Satisfies: FR-1
  - `std::vector<ImageJob> enumerate(const std::filesystem::path& dir, int max_depth = 1)`. Skips symlinks by default. Returns sorted by path for determinism.

- [ ] **M1-03** — CR3 file validation
  - Deps: M1-02
  - Size: S
  - Satisfies: FR-1
  - Validate by extension + magic bytes. Malformed files are logged and skipped, not aborted.

- [ ] **M1-04** — Error translation at boundary
  - Deps: M1-02
  - Size: S
  - Satisfies: NFR-7 (exception policy)
  - Filesystem exceptions caught, translated to `wildframe::ingest::IngestError`.

- [ ] **M1-05** — Unit tests for ingest
  - Deps: M1-03, M1-04, S0-12
  - Size: M
  - Satisfies: NFR-6 (coverage)
  - GoogleTest target `wildframe_ingest_test`. Covers: empty dir, dir with CR3 + non-CR3 files, nested dir at depth 0 and 1, unreadable file, invalid path.

---

## Module 2 — `wildframe_raw` (FR-2)

- [ ] **M2-01** — LibRaw preview extraction API
  - Deps: S0-03
  - Size: M
  - Satisfies: FR-2
  - `PreviewImage extract_preview(const std::filesystem::path& raw_path)`. Returns largest embedded JPEG as decoded RGB buffer + dimensions. RAII wrapper around `LibRaw` handle.

- [ ] **M2-02** — Full-decode crop API (fallback)
  - Deps: M2-01
  - Size: M
  - Satisfies: FR-2, FR-4
  - `CroppedImage decode_crop(const std::filesystem::path& raw_path, BBox crop_region)`. Used only when preview resolution is insufficient for focus scoring.

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

- [ ] **M5-01** — `DeterministicMetadata` struct
  - Deps: S0-03
  - Size: S
  - Public header: `libs/metadata/include/wildframe/metadata/deterministic_metadata.hpp`. All fields from handoff §13 deterministic-metadata list. Value type.

- [ ] **M5-02** — Exiv2 EXIF reader
  - Deps: M5-01
  - Size: M
  - Satisfies: FR-5
  - `DeterministicMetadata read_exif(const std::filesystem::path& raw_path)`. Maps Exiv2 keys to struct fields per the schema. Missing fields become `std::optional::nullopt`.

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
  - `DetectionResult detect(const PreviewImage& preview, const DetectConfig& cfg)`.

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
  - `FocusResult score(const PreviewImage& preview, const BBox& primary_subject, const FocusConfig& cfg)`.

- [ ] **M4-07** — Unit tests for focus
  - Deps: M4-06, S0-12
  - Size: M
  - Satisfies: NFR-6, NFR-2
  - Fixture-based. Must show: sharp fixture scores higher than motion-blurred fixture, no-bird fixture has keeper_score close to zero when no bbox is provided.

---

## Module 5b — `wildframe_metadata` (XMP write portion of FR-5 + FR-6)

- [ ] **M5-05** — `wildframe:*` XMP namespace registration
  - Deps: M5-03
  - Size: S
  - Satisfies: FR-5, NFR-4
  - Register namespace and field definitions with Exiv2 at startup. Mirror `docs/METADATA.md` exactly.

- [ ] **M5-06** — `wildframe_provenance:*` namespace
  - Deps: M5-05
  - Size: S
  - Satisfies: NFR-5
  - Fields per handoff §13.

- [ ] **M5-07** — `wildframe_user:*` namespace
  - Deps: M5-05
  - Size: S
  - Satisfies: FR-6
  - Fields per handoff §13.

- [ ] **M5-08** — XMP sidecar writer
  - Deps: M5-05, M5-06
  - Size: M
  - Satisfies: FR-5
  - `void write_sidecar(const std::filesystem::path& raw_path, const AiMetadata& ai, const ProvenanceMetadata& prov)`. Uses Exiv2 high-level API only. Atomic write via temp-file + rename. Preserves existing non-wildframe XMP content.

- [ ] **M5-09** — XMP sidecar reader
  - Deps: M5-05, M5-06, M5-07
  - Size: M
  - Satisfies: FR-6, FR-8
  - Returns all wildframe namespaces + user overrides. Used by GUI for filtering and the detail view.

- [ ] **M5-10** — User override write API
  - Deps: M5-07, M5-08
  - Size: S
  - Satisfies: FR-6
  - `void write_user_override(const std::filesystem::path& raw_path, const UserOverride& override)`. Does not modify the AI namespace.

- [ ] **M5-11** — Round-trip tests
  - Deps: M5-08, M5-09, M5-10
  - Size: M
  - Satisfies: NFR-5
  - Write AI metadata → read back → verify equality. Same for overrides. Verify that existing non-wildframe XMP keys survive a write.

---

## Module 6 — `wildframe_orchestrator` (wires everything)

- [ ] **M6-01** — `PipelineStage` interface for extensibility
  - Deps: S0-03
  - Size: M
  - Satisfies: NFR-3
  - Abstract base with `process(const Job&) -> StageResult`. MVP has three concrete stages: `DetectStage`, `FocusStage`, `MetadataWriteStage`. Phase 2+ can add stages without touching the orchestrator core.

- [ ] **M6-02** — Job queue
  - Deps: M6-01
  - Size: M
  - Thread-safe FIFO. MVP is single-consumer, but the interface supports multi-consumer for Phase 2+.

- [ ] **M6-03** — Sequential pipeline execution
  - Deps: M6-02, M1-05, M2-04, M5-04, M3-07, M4-07, M5-11
  - Size: M
  - Satisfies: §12 (data flow)
  - Worker thread drains the queue. Per job: ingest → raw → metadata (EXIF read) → detect → focus → metadata (XMP write).

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

- [ ] **M6-07** — Unit + integration tests for orchestrator
  - Deps: M6-05, M6-06
  - Size: M
  - Satisfies: NFR-6
  - Integration test: run the full pipeline over all S0-12 fixtures, verify all produced XMP sidecars and the batch manifest.

- [ ] **M6-08** — Cooperative batch cancellation
  - Deps: M6-03
  - Size: M
  - Satisfies: FR-9
  - Orchestrator exposes a `cancel()` method. When set, the worker finishes the current per-image step, writes any partial output for that image if stage boundaries allow, stops dispatching new work, records `status: cancelled` in the manifest, and returns. No forceful thread termination.

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
  - Cancel button in the progress view invokes the orchestrator's `cancel()`. UI reflects `cancelling…` state until the worker confirms completion, then shows partial-results summary.

- [ ] **M7-10** — Re-analysis prompt dialog
  - Deps: M7-01, M5-09, S0-21
  - Size: M
  - Satisfies: FR-10
  - On batch start, scan for existing `wildframe:*` sidecars. If any exist, show modal with three options (skip already-analyzed, overwrite all, cancel) and a count of how many files are affected. Per-file override is exposed later from the detail view (part of M7-07). Decision recorded to manifest and `wildframe_user:reanalysis_policy_used`.

- [ ] **M7-11** — Trim GUI link line to architectural contract
  - Deps: M7-01
  - Size: S
  - Satisfies: NFR-3, NFR-6
  - [src/CMakeLists.txt](../src/CMakeLists.txt) currently links the stub `wildframe` executable against all six static libraries. [docs/ARCHITECTURE.md §1](ARCHITECTURE.md) specifies the GUI depends only on `wildframe::orchestrator` and `wildframe::metadata`; the other four are indirect. Once M7-01 introduces real GUI code, remove the direct links to `wildframe::ingest`, `wildframe::raw`, `wildframe::detect`, `wildframe::focus` so the module boundary is enforced by the build, not just documented. Identified by the 2026-04-21 Sprint 0 infrastructure review §5.

---

## Integration / Validation

- [ ] **I-01** — End-to-end smoke test
  - Deps: M6-07, M7-08
  - Size: S
  - Satisfies: §16 (MVP readiness)
  - Run the GUI on all S0-12 fixtures. Verify sidecars, manifest, and filterability.

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
