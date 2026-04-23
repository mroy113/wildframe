---
document_type: project_handoff
project_name: wildframe
status: mvp_ready_for_decomposition
primary_goal: >
  Build Wildframe, an AI-assisted desktop application that analyzes RAW bird photos,
  generates useful metadata for culling and search, and integrates with existing
  photo workflows (Lightroom Classic).
intended_recipient: software_project_planning_agent
source_context: >
  Derived from customer/engineering scoping conversation. Defines a C++ source-code
  deliverable targeting an average-skilled embedded C++ developer as the maintainer.
  Maintainability is the primary non-functional concern for every decision.
---

# Project Handoff: Wildframe — AI-Assisted Bird Photo Metadata Engine

## 1. Executive Summary

Wildframe is a desktop application that analyzes folders of RAW bird photographs and writes back useful metadata for culling, search, and later workflow automation.

The MVP user need is:

1. Detect whether a bird is present in the frame.
2. Determine whether the bird is in focus.
3. Produce a keeper score that helps triage a burst-heavy shoot.

The broader product direction is to treat image analysis as a metadata enrichment pipeline, not a pass/fail culling tool. Once the system can reliably detect and crop birds, it can attach richer metadata — species, counts, action/context tags, quality scores — via additional pipeline stages.

---

## 2. Product Vision

### Core product
AI-assisted wildlife photo intelligence tool focused first on birds.

### Initial value proposition
Given a directory of RAW photos, automatically infer structured metadata that supports culling, filtering, ranking, and later species/context search.

### Long-term value proposition
Transform large bird-photo libraries into searchable, metadata-rich collections. Example future queries:
- sharp in-flight photos
- likely warblers from this trip
- high-confidence osprey over water
- best image from each burst
- photos that likely match an eBird checklist by place/time

---

## 3. Delivery Model

**Wildframe is delivered as C++ source code to a customer whose engineering team is an average-skilled C++ developer with mostly embedded experience.** That audience shapes every downstream choice:

- CMake build system with vcpkg dependency management (manifest mode, pinned versions).
- Pitchfork repository layout (see Section 11) — no local conventions to learn.
- Zero framework magic — standard headers, standard idioms, minimal template metaprogramming.
- Each internal module compiled as a static library with a small, documented header surface.
- LLDB is the primary debugger; debug builds preserve symbols across static library boundaries so the debugger can step through calls into any module.
- Zero Python in the shipped product. All ML inference is performed by C++ code loading pre-trained `.onnx` model files.
- Third-party dependencies are all well-established, CMake-integrated, and available in vcpkg.

### Primary platform
- **macOS, Intel silicon** for initial development (customer's current hardware).
- **macOS, Apple Silicon** as the primary performance target for release. The architecture must be device-agnostic so the switch is a runtime config change, not a port.

---

## 4. Scope

### In scope for MVP
- Analyze a user-chosen directory of RAW images.
- **CR3 format only** for MVP (LibRaw's API generalizes to additional formats later with no code change).
- Detect whether one or more birds are present; output bounding boxes and confidences.
- Estimate focus / motion blur on the primary bird subject.
- Produce a composite keeper score.
- Write per-image metadata to XMP sidecars next to the RAW files.
- Write a per-batch JSON manifest capturing the analysis run and provenance.
- Provide a Qt GUI for selecting directories, running analysis, reviewing results, filtering to likely keepers / likely no-bird frames, and overriding AI outputs.

### Out of scope for MVP
- RAW formats other than CR3 (CR2, NEF, ARW, RAF, ORF, RW2, DNG — Phase 2).
- Species identification (Phase 2).
- Eye/head-specific focus scoring (Phase 2).
- Action/context tags — in-flight, perched, feeding, etc. (Phase 2+).
- Obstruction detection (Phase 2+).
- Burst deduplication / best-of-burst ranking (Phase 4).
- Lightroom Classic plugin (Phase 3).
- raw-ingest subprocess integration in the GUI (Phase 3).
- Custom model training — requires Python sidecar (Phase 2).
- Full image editing or DAM replacement.
- Real-time / in-camera operation.
- Mobile support.
- Windows / Linux support in v1 (code must not obstruct future ports).
- Telemetry, analytics, or crash reporting of any kind — Wildframe is fully local; the only network activity is model weight download at build time.
- Auto-update mechanism.
- Installer, code signing, or notarization — delivery is source code, not a distributable macOS binary.
- Internationalization / localization (English only).
- Accessibility features beyond Qt's defaults.

---

## 5. Technology Stack

| Concern | Choice | Notes |
|---|---|---|
| Language | **C++20** | Locked. Exceptions allowed and expected; see exception policy in `docs/STYLE.md`. |
| Build | CMake (≥3.24) | Per-module `add_library(... STATIC ...)`. |
| Dependencies | vcpkg | Manifest mode, pinned versions, reproducible from clean checkout. |
| Repository layout | **Pitchfork** | See Section 11. |
| RAW decode + embedded preview extraction | **LibRaw** | MVP uses CR3; LibRaw handles the others for Phase 2. |
| EXIF read + XMP sidecar write | **Exiv2** | C++ native. One library covers deterministic metadata reads and sidecar writes. |
| Image processing + classical focus metrics | **OpenCV** | Laplacian variance, FFT, colorspace, crop/resize. |
| ML inference | **ONNX Runtime (C++ API)** | Loads `.onnx` files. CPU EP baseline, CoreML EP for GPU / Neural Engine acceleration. |
| GUI | **Qt 6 Widgets** | Not QML. Familiar to industrial/embedded developers. |
| JSON | **nlohmann/json** | Header-only, de facto. Used for batch manifests. |
| TOML (runtime config) | **tomlplusplus** | Header-only, C++17+, vcpkg-available, clang-tidy-clean. Used for the user-facing runtime configuration file (keeper-score weights, detection thresholds, EP selection, output paths). |
| Logging | **spdlog** | Chosen over Boost.Log to avoid pulling in the Boost ecosystem for one feature. Smaller surface area, better clang-tidy profile, faster. |
| Testing | **GoogleTest** | Per-module test targets. |
| Formatting | **clang-format** | Enforced in CI. |
| Static analysis | **clang-tidy** | Zero findings required. See NFR-8. |
| Debugger | **LLDB** | macOS native. |

### GPU / acceleration strategy

MVP uses **ONNX Runtime with the CoreML Execution Provider**. This provider dispatches to:
- **Intel Mac**: Metal-backed GPU (discrete AMD where present, Intel integrated otherwise), CPU fallback.
- **Apple Silicon**: Apple Neural Engine + GPU + CPU, managed by CoreML.

No code change is required when moving between machines — EP selection is a runtime flag. CPU EP remains the fallback for debugging and machines where CoreML is unavailable.

Classical CV work (focus scoring, FFT) stays on CPU via OpenCV. If benchmarking identifies a hotspot, OpenCV's OpenCL backend can be wired in without changing call sites.

### Threading strategy (MVP)

- Qt main thread owns the UI.
- A single background worker thread owns the analysis pipeline and drains a job queue.
- ONNX Runtime handles intra-op parallelism and GPU dispatch internally; no per-image application-level threading in v1.
- If throughput becomes a bottleneck, a producer–consumer pipeline (decode → infer → score → write) can be added without changing module boundaries.

---

## 6. Relationship to raw-ingest

`raw-ingest` is a separate Python CLI tool at `~/Code/raw-ingest` that the customer already owns. It offloads SD cards into a date+location-structured library under `~/Pictures/`, writes per-run JSON manifests, and integrates with eBird for location suggestions.

**Wildframe is loosely coupled to raw-ingest.** Wildframe's input contract is *a directory containing RAW files* — nothing more. It does not parse raw-ingest manifests, does not depend on raw-ingest's library layout, and does not require raw-ingest to run.

Future integration (Phase 3+): the Wildframe GUI may invoke raw-ingest as a subprocess to offer an "ingest then analyze" flow. That integration is out of scope for MVP.

---

## 7. Primary User Goals

- Reduce time spent manually culling large burst-heavy bird photo sets.
- Avoid keeping obvious non-bird frames.
- Surface likely sharp keepers faster.
- Produce metadata that supports future search / filtering workflows.
- Preserve compatibility with the existing RAW editing workflow (Lightroom, DxO, Photo Mechanic).

---

## 8. Functional Requirements

### FR-1: Input handling
- Accept a user-selected directory. Recurse one level by default (configurable).
- **MVP supports Canon CR3 only.** LibRaw's API is format-agnostic, so extending to CR2/NEF/ARW/RAF/ORF/RW2/DNG in Phase 2 is an acceptance-test change, not a code change.
- Never modify or overwrite source RAW files. All writes are sidecar or separate output files.

### FR-2: Preview extraction
- For each RAW file, extract the largest embedded JPEG preview via LibRaw for fast inference.
- Full-resolution RAW decode is used only when the primary subject bbox is small enough that classical focus metrics benefit from higher resolution.

### FR-3: Bird presence detection
- Run ONNX-backed object detection on the preview image.
- Produce: `bird_present` (bool), `bird_count` (int), `bird_boxes[]` (xywh + per-box confidence), `detection_confidence` (max confidence across boxes).
- Select a `primary_subject_box` by max confidence; ties broken by bbox area.

### FR-4: Focus and quality scoring
- Crop the preview (or a RAW-decoded region if preview resolution is insufficient) to the primary subject box.
- Compute:
  - `focus_score` — normalized variance of Laplacian on the bird crop.
  - `motion_blur_score` — FFT high-frequency energy ratio.
  - `subject_size_percent` — bbox area / image area.
- Combine into a `keeper_score` via a documented, tunable weighted formula. Penalize edge clipping.

### FR-5: Metadata generation and persistence
- Write one XMP sidecar per analyzed RAW file (`photo.CR3.xmp`) containing:
  - AI metadata (detection + focus/keeper scores).
  - Provenance (model name, model version, pipeline version, analysis timestamp).
  - Namespaced custom fields (`wildframe:*`) for anything outside standard XMP.
- Write one per-batch JSON manifest to a run-scoped folder capturing every image processed, inputs, outputs, errors, and timing.
- **XMP sidecars are the source of truth.** The JSON manifest is an append-only run log.

### FR-6: Review and override (MVP minimum)
- GUI shows the analyzed batch as a thumbnail grid with filters: keeper-score range, bird-present toggle, detection-confidence range.
- Clicking a thumbnail opens a detail view showing the preview, drawn bbox, and scores.
- User can override `bird_present` and mark approve/reject; user can add a free-text note.
- Overrides are written back to the XMP sidecar under a distinct namespace (`wildframe_user:*`) so AI-generated vs user-corrected values remain separable.

### FR-7: Host app integration (deferred)
- Not built in MVP. Module boundaries must leave room for a future Lightroom Classic plugin layer.

### FR-8: Search/filter foundation
- XMP field names and types must be documented and stable so external tools (Lightroom smart collections, etc.) can filter on them.

### FR-9: Batch cancellation
- The user can cancel an in-progress batch from the GUI.
- Cancellation is **cooperative**, not forceful — the currently executing per-image pipeline stage finishes its step, then the orchestrator stops dispatching new work.
- Partial results already written (XMP sidecars, manifest rows) are retained. No rollback of successfully written sidecars.
- The batch JSON manifest's final status is recorded as `cancelled` with a cancellation timestamp.

### FR-10: Re-analysis policy
- When the user runs analysis on a directory that already contains `wildframe:*` XMP sidecars from a prior run, Wildframe must not silently overwrite them.
- Default behavior: prompt the user with batch-level options (*skip already-analyzed*, *overwrite all*, *cancel*) and allow per-file override from the detail view. Locked by S0-21.
- The chosen option is recorded in the batch manifest so re-runs are traceable, and on each affected sidecar via `wildframe_user:reanalysis_policy_used` (§13).
- The default is overridable via TOML config key `reanalysis_default` (`prompt` | `skip` | `overwrite`, default `prompt`) for headless/automated runs; see `docs/CONFIG.md` §3.1.

### FR-11: Runtime configuration
- All user-tunable values (keeper-score weights, detection confidence threshold, NMS IoU threshold, recursion depth, detector selection, execution provider selection, output paths) live in a single **TOML** configuration file loaded at application startup.
- File location is resolved in order: CLI `--config <path>` argument, then `$XDG_CONFIG_HOME/wildframe/config.toml` (or macOS equivalent), then a built-in default.
- The application ships with a fully-commented default config file documenting every field.
- Config is **startup-only** for MVP — no hot-reload.

---

## 9. Non-Functional Requirements

### NFR-1: Performance
- A batch of 500 preview-path images should complete detection + focus scoring in under 10 minutes on Intel Mac CPU, under 2 minutes on Apple Silicon with CoreML EP. Targets, not hard budgets.

### NFR-2: Accuracy
- False-negatives on clearly sharp bird frames must be rare. False-positives on birdless frames are acceptable and fixable via the review UI.
- Confidence values must be exposed in the UI and in metadata, not hidden.

### NFR-3: Extensibility
- New pipeline stages (species, action tags, eye detection) must be addable by implementing a documented C++ interface and registering the stage with the orchestrator — no changes to the orchestrator core.

### NFR-4: Interoperability
- XMP field names follow Adobe XMP conventions where they overlap with standard properties. Custom fields are namespaced under `wildframe:` and documented in `docs/METADATA.md`.

### NFR-5: Auditability
- Every written metadata record includes pipeline version, per-stage model name and version, and analysis timestamp.

### NFR-6: Maintainability (**primary** non-functional concern)

**Maintainability is the primary consideration for every development decision in Wildframe.** Where a choice trades readability for performance, cleverness, or brevity, readability wins unless a benchmark proves the trade is required.

- Prefer **self-documenting code**: descriptive variable and function names, small functions, obvious control flow. A reader should rarely need a comment to understand *what* the code is doing.
- Do **not overuse advanced C++ features** — SFINAE, CRTP, expression templates, custom allocators, heavy template metaprogramming, operator overloading beyond arithmetic types, fold expressions in hot paths — when a straightforward implementation is clear and adequate.
- Prefer concrete types over clever generic ones. Template only where genuinely reusable.
- Each module kept under ~2000 lines of implementation where practical.
- Headers carry brief Doxygen-style comments on public interfaces only. Implementation comments explain *why*, never *what*.
- GoogleTest coverage for every module-public interface.

### NFR-7: Code Standards

- **Strict adherence to the C++ Core Guidelines** — https://isocpp.github.io/CppCoreGuidelines/
- **Strict adherence to the Google C++ Style Guide** — https://google.github.io/styleguide/cppguide.html
- Where the two documents disagree, the project maintains `docs/STYLE.md` recording which guide wins on the disputed point and why. Every such exception is listed there. Typical disputed points include exception policy, header ordering, and smart-pointer conventions — resolve them at kickoff.
- **Exception policy** (to be formalized in `docs/STYLE.md` at kickoff):
  - Exceptions are allowed and expected, following Core Guidelines (E-series) rather than the historical Google ban.
  - Use exceptions for exceptional conditions only, never for control flow.
  - RAII throughout. Every owning type has a destructor that releases its resource; no `new`/`delete` pairs in application code.
  - Mark functions `noexcept` where they are genuinely noexcept (move operations, destructors, small value-semantic getters).
  - Provide at least the basic exception safety guarantee on public APIs; strong guarantee where practical.
  - Third-party exceptions (Exiv2, LibRaw, ONNX Runtime, Qt) are caught at module boundaries and translated to Wildframe's own error types. No raw third-party exception types propagate across module boundaries.
  - Each module's public header documents the exception types it may throw.
- `.clang-format` enforces formatting rules; every commit must be clang-format-clean. CI rejects commits that are not.

### NFR-8: Static Analysis (clang-tidy zero findings)

- `clang-tidy` must run clean with **zero findings** across the codebase.
- Check set is broad. At minimum enable:
  - `bugprone-*`, `cert-*`, `clang-analyzer-*`, `cppcoreguidelines-*`, `google-*`, `hicpp-*`, `modernize-*`, `performance-*`, `portability-*`, `readability-*`, `misc-*`.
- A small set of known-noisy checks may be excluded in `.clang-tidy` with a justification comment for each exclusion.
- **Suppression policy:**
  - No `NOLINT`, `NOLINTNEXTLINE`, or `NOLINTBEGIN/END` without an immediately adjacent comment explaining *why* the suppression is legitimate.
  - For every legitimate finding, the developer must first consider at least one alternative implementation that eliminates the finding. **Suppression of a legitimate finding is a last resort, not a default.**
  - Only confirmed false positives may be suppressed, and the suppression comment must describe why it is a false positive.
- CI rejects any PR that introduces new clang-tidy findings or new unexplained suppressions.

### NFR-9: Thread safety contracts

- Each module's public header must document the concurrency contract of its API. Allowed contracts:
  - **single-threaded** — must be called from one thread; caller is responsible.
  - **thread-compatible** — distinct instances may be used from distinct threads; a single instance must not.
  - **thread-safe** — instance may be used concurrently from multiple threads.
- The GUI must not call single-threaded or thread-compatible modules from a thread other than the one the instance was created on. Violations are caught by `assert`s in debug builds.
- `wildframe_metadata` XMP reads (for GUI thumbnail population) and writes (for orchestrator pipeline output) may race; the module documents and enforces its own serialization strategy.

---

## 10. Software Components

Each component is delivered as a CMake static library target. Header interfaces are the only coupling. LLDB must be able to step from any module into any other module in a debug build.

### Module 1: `wildframe_ingest`
- Directory enumeration, file discovery, RAW format validation (CR3 only in MVP).
- Produces `ImageJob` records (file path, detected format, file size, basic EXIF snippet).

### Module 2: `wildframe_raw`
- LibRaw wrapper: embedded preview extraction, optional full-decode crop.
- Does *not* own Exiv2 — all Exiv2 use (EXIF read and XMP sidecar write) is consolidated in `wildframe_metadata` so one module owns the library.

### Module 3: `wildframe_detect`
- ONNX Runtime wrapper for object detection.
- Default model: **YOLOv11 (COCO-80, bird class)** exported to ONNX.
- Pluggable alternative via config: **MegaDetector v6**.
- Outputs: `DetectionResult` with boxes, confidences, selected primary subject.

### Module 4: `wildframe_focus`
- OpenCV-based classical focus and motion-blur scoring on the primary subject crop.
- Outputs: `FocusResult` with focus score, motion blur score, subject size, keeper score.

### Module 5: `wildframe_metadata`
- Sole owner of Exiv2.
- EXIF read (deterministic metadata) from RAW files.
- XMP sidecar read/write using Exiv2's high-level API (forbid hand-assembled XMP).
- `wildframe:*`, `wildframe_provenance:*`, `wildframe_user:*` namespace definitions and schema versioning.
- Provenance field population.

### Module 6: `wildframe_orchestrator`
- Owns the job queue and pipeline order.
- Stage registration interface for Phase 2+ additions.
- Per-batch JSON manifest writer (nlohmann/json).
- Error isolation per image — one bad file does not abort the batch.

### Module 7: `wildframe_gui`
- Qt 6 Widgets application.
- Directory picker, batch runner, progress view, thumbnail grid, detail view, filter controls, override UI.
- Links against all upstream static libraries.

### Future modules (Phase 2+, not in MVP)
- `wildframe_species` — species classification.
- `wildframe_feedback` — review overrides captured as training-ready labels.
- `wildframe_lightroom` — Lightroom Classic integration layer.

---

## 11. Repository Layout (Pitchfork)

Wildframe follows the [Pitchfork Layout](https://github.com/vector-of-bool/pitchfork) for C++ projects. Adopting a widely-documented layout eliminates local conventions that a new maintainer would otherwise have to learn.

```
wildframe/
├── CMakeLists.txt              # top-level project definition
├── CMakePresets.json           # debug / release / tidy / asan presets
├── vcpkg.json                  # dependency manifest (pinned versions)
├── .clang-format               # formatting rules (enforced in CI)
├── .clang-tidy                 # static analysis rules (zero findings required)
├── libs/                       # internal static libraries, one per module
│   ├── ingest/
│   │   ├── include/wildframe/ingest/   # public headers
│   │   ├── src/                        # implementation
│   │   ├── tests/                      # GoogleTest target
│   │   └── CMakeLists.txt
│   ├── raw/
│   ├── detect/
│   ├── focus/
│   ├── metadata/
│   └── orchestrator/
├── src/                        # the Wildframe GUI executable
│   └── CMakeLists.txt
├── data/                       # non-code assets (default config, schema files)
│   └── xmp_schema/
├── tests/                      # cross-module integration tests
├── tools/                      # build-time scripts
│   └── fetch_models.cmake      # downloads ONNX models with SHA256 verification
├── docs/                       # project documentation
│   ├── STYLE.md                # resolved conflicts between Core Guidelines and Google style; exception policy
│   ├── ARCHITECTURE.md         # module dependency diagram and rationale
│   ├── METADATA.md             # wildframe:* XMP field reference
│   └── LICENSING.md            # Qt LGPL notes, model-weight fetch URLs and upstream licenses
├── external/                   # vendored third-party code, if any
├── cmake/                      # reusable CMake helpers and find-modules
└── build/                      # out-of-source build directory (gitignored)
```

### Model assets — fetch at build time, do not commit

ONNX model weights are **not committed to the repository**. The `tools/fetch_models.cmake` script downloads them from upstream release URLs during CMake configure, verifies each file against a pinned SHA256, and caches them under `build/_models/`. This keeps the repo lean, makes the model version a reviewable part of the codebase (URL + hash both live in version control), and sidesteps redistribution licensing concerns since the weights are never republished from our tree.

---

## 12. Data Flow (MVP)

1. User picks a directory in the Qt GUI.
2. `wildframe_ingest` enumerates RAW files and creates `ImageJob` records.
3. `wildframe_orchestrator` queues jobs; the background worker drains the queue.
4. Per image:
   1. `wildframe_raw` extracts the embedded JPEG preview from the CR3.
   2. `wildframe_metadata` reads deterministic EXIF from the CR3.
   3. `wildframe_detect` runs ONNX detection on the preview, produces boxes + primary subject.
   4. `wildframe_focus` crops to the primary subject, produces focus/keeper scores.
   5. `wildframe_metadata` writes AI + provenance fields to the XMP sidecar.
   6. Orchestrator appends a row to the batch JSON manifest.
5. GUI refreshes the thumbnail grid as results arrive.
6. User reviews, filters, overrides. Overrides are written back into the same XMP sidecar under `wildframe_user:*`.

---

## 13. Metadata Schema (MVP)

### Deterministic metadata (read from EXIF, mirrored into sidecar for convenience)
- `file_path`, `file_name`, `capture_datetime`, `camera_model`, `lens_model`, `focal_length`, `aperture`, `shutter_speed`, `iso`, `gps_lat`, `gps_lon`, `image_dimensions`.

### AI metadata (XMP namespace `wildframe:`)
- `bird_present` (bool)
- `bird_count` (int)
- `bird_boxes[]` (array of {x, y, w, h, confidence})
- `primary_subject_box` (reference to best box)
- `detection_confidence` (float)
- `focus_score` (float, 0–1)
- `motion_blur_score` (float, 0–1)
- `subject_size_percent` (float, 0–1)
- `keeper_score` (float, 0–1)

### Provenance (namespace `wildframe_provenance:`)
- `analysis_timestamp`, `pipeline_version`, `detector_model_name`, `detector_model_version`, `focus_algorithm_version`.

### User override (namespace `wildframe_user:`)
- `bird_present_override` (enum: `Present`, `Absent`, `Unset` — default `Unset`).
- `keeper_override` (enum: `Approved`, `Rejected`, `Unset` — default `Unset`).
- `user_note` (string, optional).
- `override_timestamp` (ISO-8601 timestamp of the most recent user change).
- `reanalysis_policy_used` (enum: `PromptSkipped`, `PromptOverwrote`, `PerFile` — recorded when a re-analysis affected this file; see FR-10).

---

## 14. Phased Roadmap

- **Phase 1 (MVP)** — modules 1–7 as defined in Section 10, CR3 only.
- **Phase 2** — additional RAW formats (CR2/NEF/ARW/RAF/ORF/RW2/DNG), species top-k classification, eye/head detection, feedback capture, training sidecar (Python, external to delivery).
- **Phase 3** — Lightroom Classic integration layer; raw-ingest subprocess integration in GUI.
- **Phase 4** — burst dedup / best-of-burst, action/context tags, checklist-aware reranking.
- **Phase 5** — semantic search UX, eBird correlation, rarity flagging.

---

## 15. Risks and Design Cautions

### Product
- Focus is subjective. Keeper-score weights will need user-facing tuning; design the formula as documented, adjustable configuration from day one.
- Detection false-positives on statues/signs/distant non-birds. Mitigation: MegaDetector is pluggable as an alternative detector.
- Small birds, occlusion, motion blur, cluttered habitats — no model handles these perfectly.

### Technical
- Preview-path inference may lose detail on very small subjects. The full-decode fallback for focus scoring is expensive; keep it gated by subject size.
- XMP sidecar writes can corrupt existing sidecars if not careful. Exiv2 handles this correctly via its high-level API; forbid hand-assembled XMP.
- ONNX Runtime version drift across vcpkg updates. Pin the exact version in the manifest.
- **Qt 6 licensing**: MVP uses Qt via LGPL with dynamic linking. Static-linking Qt requires a commercial license. Document this in `docs/LICENSING.md` and keep Qt as the only dynamically-linked dependency in the delivered binary.
- **clang-tidy policy** may slow early development as the team tunes the check set. Budget time in Sprint 0 for finalizing `.clang-tidy` against a representative sample of module code.

### Data
- Model licensing: confirm YOLOv11 and MegaDetector redistribution terms. Because weights are fetched at build time from upstream, the customer redistributes only URLs and hashes, not weights, which simplifies the licensing posture.

---

## 16. Resolved Design Decisions (from scoping conversation)

1. Product name: **Wildframe**.
2. Delivery: **C++ source code**, targeting an average-skilled embedded C++ developer as the maintainer.
3. Primary platform: **macOS Intel** for development, **macOS Apple Silicon** as the release performance target.
4. Build: **CMake + vcpkg** (manifest mode).
5. Repository layout: **Pitchfork**.
6. GUI: **Qt 6 Widgets** (LGPL, dynamic linking).
7. ML inference: **ONNX Runtime with CoreML Execution Provider**, CPU EP fallback.
8. **No Python** in the MVP delivery. Models are pre-trained and shipped via build-time fetch of `.onnx` files.
9. Model distribution: **not committed** — fetched at build time with pinned SHA256 from upstream URLs.
10. RAW format support in MVP: **Canon CR3 only**.
11. Metadata persistence: **XMP sidecars as source of truth**, per-batch JSON manifest as run log. **No SQLite** in MVP.
12. Testing: **GoogleTest**.
13. Debugger: **LLDB**.
14. Module structure: **one static library per module**, debuggable across boundaries.
15. raw-ingest: **loosely coupled**. Wildframe's input contract is "a directory containing RAW files."
16. **Maintainability is the primary non-functional concern** for every decision.
17. **Strict C++ Core Guidelines + Google C++ Style Guide** compliance; conflicts resolved in `docs/STYLE.md`.
18. **clang-tidy zero findings** required; suppressions only for confirmed false positives, each with a justification comment.
19. **C++20** locked (no C++17 fallback).
20. **Exceptions allowed** per Core Guidelines, with policy formalized in `docs/STYLE.md`; third-party exceptions translated at module boundaries.
21. Logging: **spdlog** (Boost.Log considered and rejected to avoid pulling in Boost solely for logging).
22. Runtime config file format: **TOML** via tomlplusplus.
23. Wildframe versioning: **semver**, git-tagged `v<MAJOR>.<MINOR>.<PATCH>`. Pre-1.0: breaking changes allowed in minor bumps.
24. macOS deployment target: **macOS 13 (Ventura)** minimum. Toolchain: Apple Clang ≥ 15, CMake ≥ 3.24, Ninja, vcpkg pinned commit.
25. Output paths (overridable via TOML): spdlog logs → `~/Library/Logs/Wildframe/wildframe.log` (daily rotation, 14-day retention); batch JSON manifests → `~/Library/Application Support/Wildframe/batches/<timestamp>.json`.
26. Re-analysis default: **prompt per batch** (*skip already-analyzed* / *overwrite all* / *cancel*), with per-file override available from the detail view.

---

## 17. Handoff to Planning Agent

This document is ready to be decomposed into development tasks. The planning agent should:

1. Produce a per-module task breakdown aligned with the seven MVP modules in Section 10.
2. Define module-level acceptance criteria tied to the FRs in Section 8 and NFRs in Section 9.
3. Sequence implementation as a **tracer bullet** (Section 18), not per-module bottom-up. The end-to-end CLI slice lands before any stage receives its real implementation; stages are then thickened one at a time while keeping the end-to-end smoke test green. The original bottom-up critical path (`wildframe_ingest` → `wildframe_raw` → `wildframe_detect` → `wildframe_focus` → `wildframe_metadata` → `wildframe_orchestrator` → `wildframe_gui`) is retained only as the module *dependency* graph, not as the implementation order.
4. Schedule **Sprint 0** tasks before any module work:
   - Pitchfork directory scaffold (Section 11).
   - `vcpkg.json` manifest with pinned versions of LibRaw, Exiv2, OpenCV, ONNX Runtime, Qt 6, nlohmann/json, tomlplusplus, spdlog, GoogleTest.
   - Top-level `CMakeLists.txt` with one empty static-lib target per module, plus the GUI executable target.
   - `CMakePresets.json` for debug, release, tidy (clang-tidy run), and asan.
   - `.clang-format` baseline derived from Google style.
   - `.clang-tidy` baseline with the check set in NFR-8; tune against a sample module.
   - `docs/STYLE.md` resolving Core Guidelines vs Google Style Guide conflicts.
   - `tools/fetch_models.cmake` with YOLOv11 URL + SHA256 placeholder entries.
   - Test fixture policy: decide between committing a small set of representative CR3 files (LFS, a few MB each) and fetching them at test-setup time the same way models are fetched. Fixtures must cover: clear bird in frame, no bird, small distant bird, motion-blurred bird, and a non-bird false-positive magnet (statue or sign).
   - CI (GitHub Actions, macOS runner) running: build, test, clang-format check, clang-tidy gate.
5. Treat YOLOv11 ONNX export URL + SHA256 + licensing confirmation as a prerequisite for `wildframe_detect`.
6. Schedule a benchmark task at the end of MVP to validate the Section 9 performance targets on both Intel and Apple Silicon.
7. Every task's Definition of Done must include: clang-format clean, clang-tidy zero-findings, GoogleTest coverage for public interfaces, and conformance to `docs/STYLE.md`.

---

## 18. Implementation Sequencing — Tracer-Bullet First

MVP is sequenced as a **tracer bullet** (vertical slice), not a per-module waterfall.

1. **Skeleton slice.** A CLI entry point drives the full pipeline end-to-end on real CR3 inputs. Expensive stages (`wildframe_raw`, `wildframe_detect`, `wildframe_focus`, and the `wildframe:`/`wildframe_user:` XMP namespace writes) ship as **stubs** that satisfy their public headers with sentinel returns. Cheap stages (`wildframe_ingest`, orchestrator skeleton, XMP provenance-only write, minimal batch manifest) ship real. At the end of the slice, one command produces a valid sidecar + manifest row for every S0-12 fixture.

2. **Thickening passes.** Each stub is replaced by its real implementation one at a time, in whatever order pays off the most signal — typically `wildframe_raw` first (unblocks real pixel data), then `wildframe_detect`, `wildframe_focus`, and full metadata writes. The end-to-end smoke test must stay green across every replacement; a thickening that regresses it is not ready.

3. **GUI layer.** Module 7 builds on top of the thickened pipeline. The CLI persists alongside the GUI as a genuine long-term entry point for headless and automated use — FR-10's `reanalysis_default` and FR-11's runtime config already scope that use.

### Constraints this does not relax

- **A stub is not a TODO.** It lives behind the same public interface the real implementation will satisfy, ships with the same Doxygen, tidies clean under the same check set, and is covered by tests that assert the sentinel behavior. Definition of Done (CLAUDE.md §4) applies in full.
- **`docs/STYLE.md §2.11` ("Do not ship speculative interface surface") is not violated.** §2.11 forbids exported surface whose only caller is a future task; tracer-bullet stubs have a live caller (the CLI) from the moment they ship.
- **Scope does not widen.** The stubs cover exactly the seven MVP modules. Phase 2+ work (species, eye detection, Lightroom) remains out of scope per §6.

### Why this shape

Solo contributor + a pipeline with real interface unknowns (ONNX output shapes, focus-metric return types, XMP round-trip quirks under Exiv2). The coordination benefit that justifies stabilizing inter-module contracts before integration does not apply to one maintainer, and the end-to-end slice surfaces interface surprises inside each sprint rather than at a big-bang integration after Module 6.
