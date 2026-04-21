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

- [ ] **S0-02** — `vcpkg.json` manifest with pinned versions
  - Deps: S0-01
  - Size: S
  - Satisfies: §5 (tech stack), NFR-6
  - Add manifest mode `vcpkg.json` at repo root. Pin: libraw, exiv2, opencv, onnxruntime, qt6-base, qt6-widgets, nlohmann-json, spdlog, gtest. Document exact pinned versions in the PR body.

- [ ] **S0-03** — Top-level `CMakeLists.txt` with empty static-lib targets
  - Deps: S0-02
  - Size: M
  - Satisfies: §10, §11, NFR-6
  - One `add_library(wildframe_<module> STATIC)` per MVP module, each with an empty `.cpp` and a public header. One `add_executable(wildframe)` for the GUI. Wire vcpkg toolchain. Must build clean with no sources.

- [ ] **S0-04** — `CMakePresets.json` with debug / release / tidy / asan
  - Deps: S0-03
  - Size: S
  - Satisfies: §3 (delivery), NFR-8
  - `debug`: `-DCMAKE_BUILD_TYPE=Debug -g`. `release`: `-DCMAKE_BUILD_TYPE=Release -O3`. `tidy`: runs clang-tidy over the compile-commands database. `asan`: debug + AddressSanitizer.

- [ ] **S0-05** — `.clang-format` baseline
  - Deps: S0-03
  - Size: S
  - Satisfies: NFR-7
  - Derive from Google style. Document deviations in `docs/STYLE.md`. Add `format` and `format-check` CMake targets.

- [ ] **S0-06** — `.clang-tidy` baseline and tuning
  - Deps: S0-05
  - Size: M
  - Satisfies: NFR-8
  - Enable the check set in NFR-8. Run against a representative sample (write a 200-line throwaway module). Disable only checks that are unequivocally noisy; document each disabled check inline in `.clang-tidy` with a one-line justification.

- [ ] **S0-07** — `docs/STYLE.md`
  - Deps: S0-01
  - Size: M
  - Satisfies: NFR-7
  - Resolve Core Guidelines vs Google Style conflicts for: exception policy (per handoff §NFR-7), header ordering, smart-pointer conventions, naming (CamelCase vs snake_case for types/functions/variables), use of `auto`, RAII patterns, `noexcept` policy. Every resolution cites why.

- [ ] **S0-08** — `docs/LICENSING.md`
  - Deps: S0-01
  - Size: S
  - Satisfies: §15 (risks)
  - Document Qt 6 LGPL + dynamic-linking obligation, Wildframe's license choice (needs customer decision), upstream licenses for LibRaw, Exiv2, OpenCV, ONNX Runtime, spdlog, nlohmann/json, GoogleTest. Document model-weight URLs + upstream licenses for YOLOv11 and MegaDetector.

- [ ] **S0-09** — `docs/ARCHITECTURE.md`
  - Deps: S0-01
  - Size: M
  - Satisfies: §10, NFR-3
  - Module dependency graph (Mermaid or ASCII). Per-module one-paragraph purpose. Data-flow walkthrough (from §12 of the handoff). Stage-registration extension point for Phase 2+.

- [ ] **S0-10** — `docs/METADATA.md`
  - Deps: S0-01
  - Size: S
  - Satisfies: FR-5, FR-8, NFR-4
  - Document every field in the three namespaces (`wildframe:`, `wildframe_provenance:`, `wildframe_user:`). For each: name, type, range, whether AI- or user-written, and a one-line purpose. This file is the contract external tools (Lightroom smart collections) rely on.

- [ ] **S0-11** — `tools/fetch_models.cmake`
  - Deps: S0-03
  - Size: M
  - Satisfies: §11 (model assets), §15 (licensing)
  - CMake module called during configure. Downloads YOLOv11 ONNX from a pinned URL to `build/_models/yolov11.onnx`. Verifies SHA256. Caches on re-run. Idempotent. Fails configure with a clear message if the download fails.

- [ ] **S0-12** — Test fixture policy + initial CR3 fixture set
  - Deps: S0-01
  - Size: M
  - Satisfies: NFR-2
  - Customer decision: commit CR3 fixtures under Git LFS, or fetch at test-setup time (same pattern as models)? Document the chosen approach. Acquire fixtures covering: (1) clear bird in frame, (2) no bird, (3) small distant bird, (4) motion-blurred bird, (5) non-bird false-positive magnet (statue/sign). Each ≤10 MB if possible.

- [ ] **S0-13** — CI: GitHub Actions, macOS runner
  - Deps: S0-04, S0-05, S0-06
  - Size: M
  - Satisfies: NFR-7, NFR-8
  - Jobs: configure + build (debug and release), run `ctest`, run format-check, run clang-tidy gate. All required to pass before merge. Cache vcpkg and `build/_models/`.

- [ ] **S0-14** — spdlog global logger initialization
  - Deps: S0-02, S0-03, S0-17
  - Size: S
  - Satisfies: NFR-5 (auditability)
  - Define a `wildframe_log` library or inline header in `wildframe_orchestrator` providing a configured spdlog sink (stdout + rotating file). Log format includes timestamp, level, and module tag. Level thresholds and destinations per S0-17.

- [ ] **S0-15** — Developer environment bootstrap
  - Deps: S0-01, S0-02
  - Size: M
  - Satisfies: NFR-6 (maintainability)
  - `docs/DEV_SETUP.md` + a `tools/bootstrap.sh` script covering: Xcode command-line tools, Homebrew, CMake ≥3.24, Ninja, vcpkg clone + bootstrap (pinned to a specific commit for reproducibility), first `cmake --preset debug` + build. Must take a clean macOS machine to "first build passing" with one command.

- [ ] **S0-16** — User-facing README.md
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

- [ ] **S0-17** — Logging policy
  - Deps: S0-01
  - Size: S
  - Satisfies: NFR-5
  - Section added to `docs/STYLE.md` specifying: level semantics (trace/debug/info/warn/error/critical), when to use each, default level for release vs debug builds, log file path and rotation policy (consumes S0-19 output), module tag conventions. Agents follow this spec when adding log lines.

- [ ] **S0-18** — TOML runtime config specification
  - Deps: S0-01, S0-02
  - Size: M
  - Satisfies: FR-11
  - `docs/CONFIG.md` documenting every configurable field: name, type, range, default, which FR/NFR it relates to. Ship a fully-commented default `data/config.default.toml`. Specify the resolution order (CLI arg → XDG path → built-in default). Config is startup-only for MVP — no hot-reload.

- [ ] **S0-19** — Output path implementation
  - Deps: S0-18
  - Size: S
  - Satisfies: FR-5, FR-11
  - **Locked defaults:**
    - spdlog application log: `~/Library/Logs/Wildframe/wildframe.log` with daily rotation, 14-day retention.
    - Batch JSON manifest: `~/Library/Application Support/Wildframe/batches/<ISO-8601-timestamp>.json`.
    - Both paths are overridable via TOML config keys `log_path` and `manifest_dir`.
  - Implement path-resolution helpers. Record in `docs/CONFIG.md`. Cross-link from `README.md` (see S0-16).

- [ ] **S0-20** — macOS deployment target + toolchain minimums
  - Deps: S0-01
  - Size: S
  - Satisfies: NFR-6 (reproducibility), §3 (platform)
  - **Decided:** minimum macOS **13 (Ventura)**. Minimum Apple Clang: 15. Minimum CMake: 3.24. Ninja required. vcpkg pinned to a specific commit. Enforced by top-level `CMakeLists.txt` via `CMAKE_OSX_DEPLOYMENT_TARGET=13.0` and compiler version check. Record in `docs/DEV_SETUP.md`.

- [ ] **S0-21** — Re-analysis behavior
  - Deps: S0-01, M5-09
  - Size: S
  - Satisfies: FR-10
  - **Decided:** default is **prompt-per-batch** with three batch-level options (*skip already-analyzed*, *overwrite all*, *cancel*) and a per-file override available from the detail view. Chosen option recorded in the batch manifest and on each affected sidecar via `wildframe_user:reanalysis_policy_used`. Overridable via TOML config key `reanalysis_default` for headless/automated runs.

---

## Module 1 — `wildframe_ingest` (FR-1)

- [ ] **M1-01** — `ImageJob` public struct
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

## Conventions for adding tasks

- New tasks are appended at the end of the relevant section.
- IDs are never reused or renumbered.
- Removed tasks are struck through with a one-line reason, not deleted, so history stays traceable.
