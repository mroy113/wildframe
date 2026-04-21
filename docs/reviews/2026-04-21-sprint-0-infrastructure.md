# Project Health Review — Sprint 0 Infrastructure

## Metadata

- **Milestone:** Sprint 0 infrastructure (S0-01 through S0-13) complete; spec/docs tasks S0-14 through S0-21 still pending. `/review` was invoked with an empty milestone argument; slug chosen to scope the review to the infrastructure half of Sprint 0, i.e. "everything needed before real module code can land."
- **Date:** 2026-04-21
- **Commit reviewed:** `2fd57335917502b874b8529df5fccc24ca7dd417` (branch `ci/s0-13-github-actions`, not yet merged to `main`; most recent `main` commit is `a0d3d13`).
- **Reviewer agent:** Claude Opus 4.7, review-only.
- **Scope examined:** repo root (`CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json`, `.clang-tidy`, `.clang-format`, `.gitignore`), `cmake/ClangTooling.cmake`, `tools/fetch_models.cmake`, `tools/fetch_fixtures.cmake`, `.github/workflows/ci.yml`, all seven module scaffolds under `libs/` and `src/`, every file under `docs/` that S0-07 through S0-12 produced, and `CONTRIBUTING.md`. Not examined: unbuilt artifacts, external CI run history on GitHub, actual hash verification of the pinned release assets.

---

## 1. Plan Fidelity

### Matches plan
- Repo layout matches handoff §11 Pitchfork scaffold: six static-lib modules under [libs/](../../libs/), GUI under [src/](../../src/), top-level [CMakeLists.txt](../../CMakeLists.txt) wires them per §10.
- Dependency manifest [vcpkg.json](../../vcpkg.json) pins every library called out in handoff §5 (libraw, exiv2, opencv4, onnxruntime, qtbase, nlohmann-json, tomlplusplus, spdlog, gtest) with explicit `overrides` entries, against a `builtin-baseline` SHA.
- `CMakePresets.json` provides `debug` / `release` / `tidy` / `asan` per S0-04; `tidy` preset wires `CMAKE_CXX_CLANG_TIDY` which [cmake/ClangTooling.cmake:14-24](../../cmake/ClangTooling.cmake#L14-L24) resolves to the Homebrew keg-only clang-tidy per the env constraint.
- `.clang-tidy` enables exactly the check categories listed in handoff §NFR-8 and disables each check with an inline justification comment, per the handoff's suppression policy — see [.clang-tidy:17-92](../../.clang-tidy#L17-L92).
- Model-weights and fixture-asset policy matches handoff §11 and §15: weights and fixtures fetched at configure time via [tools/fetch_models.cmake](../../tools/fetch_models.cmake) and [tools/fetch_fixtures.cmake](../../tools/fetch_fixtures.cmake) with pinned URL + SHA256, cached under `build/_models/` and `build/_fixtures/`, both gitignored.
- XMP namespace contract in [docs/METADATA.md](../METADATA.md) mirrors handoff §13 fields and namespaces, with an explicit stability and change-policy section (§9) the handoff does not require but NFR-4 benefits from.
- Exception policy and style resolutions in [docs/STYLE.md](../STYLE.md) implement handoff §NFR-7 and §NFR-6 as specified (Core Guidelines for exceptions / smart pointers / `noexcept`, Google for naming / header order / `auto`).
- GitHub Actions CI in [.github/workflows/ci.yml](../../.github/workflows/ci.yml) covers all gates S0-13 enumerates (debug + release build, ctest, format-check, clang-tidy) on `macos-14`, with vcpkg binary cache and a hashed asset cache.

### Silent deviations from plan
- [src/CMakeLists.txt:5-13](../../src/CMakeLists.txt#L5-L13) links the stub GUI executable against **all six** static libraries. [docs/ARCHITECTURE.md:37-44](../ARCHITECTURE.md#L37-L44) specifies the GUI depends only on `orchestrator` and `metadata`; the other four are indirect through the orchestrator. Benign at stub stage (no sources means nothing leaks across the boundary), but should be trimmed when M7-01 introduces real GUI code — otherwise the module-boundary contract is weaker than ARCHITECTURE.md promises.
- [docs/STYLE.md §2.1 Notes](../STYLE.md#L94-L101) shows accessor example `box.area()` as a "short, trivial accessor" carve-out. That spelling is `snake_case`, which conflicts with the row above that says functions are `CamelCase`. Either the carve-out is meant to spell the accessor `box.Area()` (consistent with Google) or the carve-out is broader than the row implies. Documentation bug, not a code bug — no enforcement yet.
- `LICENSE` file is **not** at repo root and `vcpkg.json` still has `"license": null`, both acknowledged in [docs/LICENSING.md:51-62](../LICENSING.md#L51-L62) as "follow-on work, not in scope for S0-08." The decision (GPL-3.0-or-later) is recorded but not machine-readable. Not a silent deviation from the plan, but a silent gap in the backlog — no task ID tracks this follow-up (see §5).
- [docs/STYLE.md §4 Logging](../STYLE.md#L319-L322) is an explicit `TODO (S0-17)` placeholder. Fine as long as no module ships log lines before S0-17 lands.

### Planned but missing
- **S0-14 through S0-21** — spdlog init, dev-env bootstrap, README, logging policy, TOML config spec, output-path implementation, macOS-target gate in CMake, re-analysis behavior. None of these are committed; all are checkbox-`[ ]` in `docs/BACKLOG.md` and correctly so. Among these, **S0-20** (macOS deployment target) has a concrete, enforceable piece that could have landed here: a `CMAKE_OSX_DEPLOYMENT_TARGET=13.0` line and compiler-version check in [CMakeLists.txt](../../CMakeLists.txt). The absence is fine (the task is `[ ]`), but if it doesn't land before M1-01, module builds may silently link against a newer deployment target than §3 specifies.
- No top-level `README.md` at repo root. Tracked by S0-16, so not a surprise, but the repo is now fully public-reachable via the release-asset URLs in `fetch_models.cmake` and `fetch_fixtures.cmake`; an unfamiliar visitor arrives at a bare tree with no orientation.
- The `tests/` directory exists at [tests/](../../tests/) but is empty and has no `CMakeLists.txt`. Handoff §11 shows it reserved for cross-module integration tests; fine that it's empty pre-M6, but worth confirming nobody expects `ctest --preset debug` to find integration tests here yet.

---

## 2. Dependency Reassessment

There is effectively no in-tree code to replace at this milestone — every module is a `#pragma once` + empty namespace + one-line `.cpp` stub. Dependency reassessment is therefore limited to the build-system tooling and two candidate libraries one could hypothesize for tools already in the tree. Default recommendation per NFR-6 is **reject**.

### Candidates identified

- **Module:** build system (configure-time asset fetch).
- **In-tree code it would replace:** [tools/fetch_models.cmake](../../tools/fetch_models.cmake) (69 lines) and [tools/fetch_fixtures.cmake](../../tools/fetch_fixtures.cmake) (91 lines, plus its fixture catalog lines 72-92), both of which wrap CMake's `file(DOWNLOAD ... EXPECTED_HASH ...)` with an idempotent cache check and a `FATAL_ERROR` diagnostic.
- **Candidate library:** CMake `FetchContent` + `ExternalProject_Add`.
- **Pros vs building in-house:** built into CMake, no third-party dep, recognized idiom.
- **Cons vs building in-house:** both are designed for source trees, not opaque binary assets. `FetchContent_Declare(... URL_HASH SHA256=...)` works but lands assets inside `_deps/<name>-src/` rather than the pinned destination the fixtures doc (`docs/FIXTURES.md §1.2`) and the test wiring (future `WILDFRAME_FIXTURES_DIR` compile-definition) already depend on. Migration would churn the C++ test wiring for zero readability gain.
- **Recommendation:** **reject**. The in-tree functions are ~60 lines each, legible, and have inline diagnostic messages that a CMake stdlib equivalent wouldn't match. They also both share a structurally identical body — if a third asset fetcher appears, extract a common helper in-repo rather than adopting FetchContent.

- **Module:** build system (CI YAML).
- **In-tree code it would replace:** explicit `brew install ninja llvm autoconf autoconf-archive automake libtool` step at [.github/workflows/ci.yml:62](../../.github/workflows/ci.yml#L62) plus the hand-rolled `Bootstrap vcpkg` step at lines 64-75.
- **Candidate action:** `lukka/run-vcpkg@v11` (or `microsoft/setup-vcpkg` equivalents).
- **Pros vs building in-house:** maintained by upstream vcpkg community, handles manifest-mode baseline sync automatically.
- **Cons vs building in-house:** the current 10-line shell step is shorter than the `with:` block a third-party action would need, and it keeps the CI workflow readable by someone who has never used `lukka/run-vcpkg` before — an average embedded developer per NFR-6 §3. Also introduces a third-party supply-chain dependency in CI.
- **Recommendation:** **reject**. Current approach is explicit and legible; the action would save at most a handful of lines at the cost of opacity.

### Modules reviewed, no candidate worth considering
- `wildframe_ingest`, `wildframe_raw`, `wildframe_detect`, `wildframe_focus`, `wildframe_metadata`, `wildframe_orchestrator`: all are header-only stubs at this commit. No in-tree code exists for a library to replace. Re-do dependency reassessment after M1-01 / M2-01 / M3-01 land actual APIs.

---

## 3. NFR Drift

| NFR | State | Evidence |
|---|---|---|
| NFR-1 Performance | not yet measurable | No executable pipeline exists; the `release` preset compiles empty libraries. Benchmark task I-02 depends on full pipeline. |
| NFR-2 Accuracy | not yet measurable | Fixture corpus is partial: [tools/fetch_fixtures.cmake:72-91](../../tools/fetch_fixtures.cmake#L72-L91) pins 3 of 5 categories; fixtures 4 and 5 deferred to S0-12b per [docs/FIXTURES.md §4](../FIXTURES.md) with the explicit agent-directive "do not pick up without customer". Acceptance tests that depend on fixtures 4 and 5 will skip, not fail, once tests exist. |
| NFR-3 Extensibility | on track | [docs/ARCHITECTURE.md §4](../ARCHITECTURE.md) specifies the `PipelineStage` contract and its registration model; no stub code contradicts it yet. |
| NFR-4 Interoperability | on track | [docs/METADATA.md](../METADATA.md) is comprehensive — namespaces, URIs, field types, change policy, example sidecar. URI stability rule in §2.1 goes beyond the handoff and strengthens the contract. |
| NFR-5 Auditability | on track (pre-code) | Provenance field catalog in [docs/METADATA.md §4](../METADATA.md) covers every field handoff §13 requires plus `detection_confidence_threshold` / `detection_iou_threshold` / `execution_provider` which are additions, not drift — they strengthen reproducibility. No logging facility exists yet (S0-14/S0-17 pending). |
| NFR-6 Maintainability (**primary**) | on track | Scaffold is minimal: every `libs/<m>/CMakeLists.txt` is 8 lines, every stub source is a one-line include, every public header is a short `\file` comment + empty namespace. Top-level [CMakeLists.txt](../../CMakeLists.txt) is 46 lines. No cleverness anywhere. One concern: the stub GUI over-links six libraries instead of two (see §1 deviations), which if forgotten later erodes the module-boundary discipline NFR-6 depends on. |
| NFR-7 Code Standards | on track | [docs/STYLE.md](../STYLE.md) resolves every Core-Guidelines-vs-Google conflict the handoff enumerates. Exception policy §3 mirrors handoff §NFR-7 verbatim. `.clang-format` is vanilla Google, enforced by the `format-check` target and the CI gate. One doc bug noted in §1 (`box.area()` casing). |
| NFR-8 Static Analysis | on track with reservation | Check set matches handoff §NFR-8; zero `NOLINT` suppressions in-tree (confirmed by grep — no C++ code to suppress against yet). 13 preference-based disables in [.clang-tidy:50-92](../../.clang-tidy#L50-L92) — each has an inline justification, which is compliant, but the volume is larger than the handoff's "a small set of known-noisy checks". Worth re-auditing once the first module with real code (M1) lands so the justifications can be tested against actual call sites rather than hypothetical noise. |

### clang-tidy suppression audit
- Total in-code suppressions (`// NOLINT` / `NOLINTNEXTLINE` / `NOLINTBEGIN`): **0** across all of [libs/](../../libs/), [src/](../../src/), [tests/](../../tests/) — confirmed by the near-empty stub state, no C++ code exists to suppress against.
- Global disables in `.clang-tidy`:
  - **Category (a) duplicate-alias disables:** 25 — all `hicpp-*` / `cert-*` / `cppcoreguidelines-*` / `google-readability-function-size` that re-export a check enabled by another group. These are mechanical and correct.
  - **Category (b) preference-based disables:** 13 — `modernize-use-trailing-return-type`, `modernize-use-nodiscard`, `readability-identifier-length`, `cppcoreguidelines-avoid-magic-numbers`, `readability-magic-numbers`, `cppcoreguidelines-pro-bounds-pointer-arithmetic`, `cppcoreguidelines-pro-type-vararg`, `cppcoreguidelines-owning-memory`, `cppcoreguidelines-narrowing-conversions`, `cppcoreguidelines-avoid-non-const-global-variables`, `cppcoreguidelines-avoid-do-while`, `bugprone-easily-swappable-parameters`, `portability-avoid-pragma-once`. Each has a justification in the comment block at [.clang-tidy:50-92](../../.clang-tidy#L50-L92).
- New since last review: N/A — this is the first review.
- Warrant re-examination once real code exists: `cppcoreguidelines-pro-bounds-pointer-arithmetic`, `cppcoreguidelines-narrowing-conversions`, `bugprone-easily-swappable-parameters`. Each was disabled against hypothesized noise; once module code exists, test whether the actual noise level justifies the disable or whether a targeted NOLINT on a small set of sites would be cleaner.

### Readability spot-checks
Not meaningful at this commit — `libs/*/src/*.cpp` are all one-line `#include` stubs and `libs/*/include/wildframe/*/*.hpp` are all short `\file` Doxygen comments + an empty namespace. The only C++ in the tree that a new maintainer would actually read is [src/main.cpp](../../src/main.cpp) (6 lines). Re-do this spot-check in the next review once M1 or M2 land.

CMake readability is high: [tools/fetch_models.cmake](../../tools/fetch_models.cmake) and [tools/fetch_fixtures.cmake](../../tools/fetch_fixtures.cmake) are structurally identical and both legible in one sitting. [.github/workflows/ci.yml](../../.github/workflows/ci.yml) is heavily commented and a new contributor could understand what each step does without reading vcpkg's documentation.

---

## 4. Risk Register Update

### Risks elevated since last review
- None (first review).

### New risks discovered
- **CI first-build cost.** Handoff §15 does not call this out. The vcpkg binary-source cache in [.github/workflows/ci.yml:40](../../.github/workflows/ci.yml#L40) is `clear;x-gha,readwrite` — the very first run on any new branch pays for a cold source build of Qt 6, OpenCV, ONNX Runtime, Exiv2, and LibRaw. On `macos-14` this is likely to exceed the 180-minute `timeout-minutes` ceiling set at [ci.yml:37](../../.github/workflows/ci.yml#L37). The cache helps subsequent runs; the first PR may need special handling. Mitigation: confirm first CI run actually completes within 3 hours before S0-13 is declared green.
- **Asset hosting single point of failure.** Model weights ([tools/fetch_models.cmake:67](../../tools/fetch_models.cmake#L67)) and fixtures ([tools/fetch_fixtures.cmake:75-90](../../tools/fetch_fixtures.cmake#L75-L90)) both point at `github.com/mroy113/wildframe/releases/`. If that repo is renamed, deleted, or becomes private, every clean configure (and CI run) breaks. Not a near-term concern, but worth mentioning in [docs/DEV_SETUP.md](../DEV_SETUP.md) when S0-15 lands so a future maintainer knows to mirror.

### Risks no longer relevant
- Handoff §15 "clang-tidy policy may slow early development as the team tunes the check set." Mitigated by S0-06: the check set is already tuned against a representative sample (per [.clang-tidy:1-17](../../.clang-tidy#L1-L17) comment block) and the tuning decisions are justified in-file. Early module code will inherit the tuned set rather than fighting it.

---

## 5. Backlog Grooming

### Re-order
- None suggested. The backlog's Sprint 0 ordering is internally consistent. S0-14 depends on S0-17 (logging policy before global logger init) which matches the existing deps.

### Re-size
- None with evidence. Every completed S0 task was sized S or M and delivered as sized.

### Add
- **Proposed task ID:** S0-22.
  **Title:** Add `LICENSE` file + set `vcpkg.json` license field.
  **Rationale:** [docs/LICENSING.md §1.1](../LICENSING.md#L51-L62) explicitly calls this out as "follow-on work, not in scope for S0-08," but there is currently no backlog item tracking it. Without it, the GPL-3.0-or-later decision is legible only inside a docs file; consumers of the source tree (GitHub's license-detection UI, vcpkg's own validation, downstream packagers) see `null`. Justified by §1 / §3 finding above.
  **Deps:** S0-08.
  **Size:** S.
  **Satisfies:** §15 (risks, licensing posture).

- **Proposed task ID:** M7-00 (or a one-line note under M7-01).
  **Title:** Trim `src/CMakeLists.txt` to link only `orchestrator` + `metadata` when GUI code lands.
  **Rationale:** [src/CMakeLists.txt:5-13](../../src/CMakeLists.txt#L5-L13) currently over-links six libraries against the stub GUI. [docs/ARCHITECTURE.md §1](../ARCHITECTURE.md) specifies two. Benign at stub stage; matters once real GUI code can accidentally `#include <wildframe/detect/detect.hpp>` and bypass the orchestrator boundary. Justified by §1 finding above.
  **Deps:** M7-01.
  **Size:** S.

### Remove or mark obsolete
- None.

---

## 6. Open Ambiguities

- **vcpkg pin strategy under S0-20.** [.github/workflows/ci.yml:64-75](../../.github/workflows/ci.yml#L64-L75) pins vcpkg to `vcpkg.json`'s `builtin-baseline` at configure-time. S0-20 says the developer environment will pin "vcpkg to a specific commit." Are these the same pin (the baseline SHA)? If so, say so in [docs/DEV_SETUP.md](../DEV_SETUP.md) when it lands; if not, the two pins can drift. Customer call on which is authoritative.
- **`docs/STYLE.md §2.1` accessor casing.** `box.area()` versus `box.Area()`. Which one wins? See §1 "silent deviations" above.
- **First-run CI timing.** Has anyone yet observed a cold-cache CI run complete inside 180 minutes on `macos-14`? If not, S0-13 acceptance may need the timeout raised or the heaviest vcpkg dep (Qt 6) split into an earlier warm-up step. See §4 new risks.
- **Integration-test discovery.** Is `tests/` (currently empty) expected to host `CMakeLists.txt` that `add_subdirectory` picks up today, or only once M6 lands? Affects whether `ctest --preset debug` is expected to discover zero tests on a clean checkout (currently true) or error (not observed).

---

## Summary

Sprint 0's infrastructure half is in good shape. The scaffold faithfully implements handoff §10, §11, and §NFR-6 through §NFR-8; the dependency manifest, build presets, static-analysis configuration, licensing posture, architecture documentation, metadata schema contract, model-fetch and fixture-fetch build plumbing, and CI workflow are all present, coherent, and well-commented. No silent drift from the plan warrants reversion. The most substantive concern is the thirteen preference-based clang-tidy disables: every one is justified inline and compliant with the handoff, but the aggregate is larger than the handoff's "small set" wording contemplates, and the justifications are hypothetical until real code exists to test them against. Re-audit at the next review after M1 lands.

Before the next review, the two actions worth taking are (1) landing S0-22 (new) so the GPL-3.0-or-later decision becomes machine-readable, and (2) confirming the cold-cache CI run actually completes under the 180-minute timeout — this is the one place where S0-13 acceptance could still fail in the real world despite every local gate being green. Everything else is waiting on planned downstream tasks.
