# Project Health Review — State of `main`

## Metadata

- **Milestone:** State of `main` as of 2026-04-23. `/review` invoked with the argument "the state of main" rather than a named milestone gate, so the slug scopes to that verbatim: everything that has merged since the 2026-04-21 Sprint 0 infrastructure review. Covers the remaining Sprint 0 task closures (S0-14, S0-19, S0-21, S0-22 which also landed in the prior review window via #25), the Sprint 1 CI follow-ups that merged (S1-01 / S1-02 / S1-04 / S1-05 / S1-08), the first Module 1 code (M1-01 `ImageJob`), and the Sprint 2 tracer-bullet plan pivot (#34). Sprint 2 code has not begun.
- **Date:** 2026-04-23
- **Commit reviewed:** `f62ebe46e312c3a956f7ba41170d177a3649402d` (branch `main`, clean tree).
- **Reviewer agent:** Claude Opus 4.7, review-only.
- **Scope examined:** repo root (`CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json`, `.clang-tidy`, `.clang-format`); [cmake/ClangTooling.cmake](../../cmake/ClangTooling.cmake), [cmake/PlatformMinimums.cmake](../../cmake/PlatformMinimums.cmake); [.github/workflows/ci.yml](../../.github/workflows/ci.yml); every file under [libs/](../../libs/) and [src/](../../src/); every file under [docs/](../) that landed or changed since 2026-04-21 ([docs/STYLE.md](../STYLE.md), [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/CONFIG.md](../CONFIG.md), [docs/BACKLOG.md](../BACKLOG.md), [docs/DEV_SETUP.md](../DEV_SETUP.md)); the prior review at [docs/reviews/2026-04-21-sprint-0-infrastructure.md](2026-04-21-sprint-0-infrastructure.md); git log for the review window. Not examined: CI run history on GitHub beyond what the comments in `ci.yml` already reference, actual SHA verification of the pinned release assets, any local `build/` output.

---

## 1. Plan Fidelity

### Matches plan

- Sprint 0 is closed out. Every `[x]` in [docs/BACKLOG.md:20-167](../BACKLOG.md#L20-L167) that was open at the prior review (S0-14, S0-15, S0-16, S0-17, S0-18, S0-19, S0-20, S0-21, S0-22) has landed and checkbox-flipped inside its PR, matching the §7 working-loop rule in [CLAUDE.md](../../CLAUDE.md).
- S0-12b remains `[ ]` correctly — its agent-directive forbids pickup without customer input.
- The Sprint 1 CI follow-ups `S1-01`, `S1-02`, `S1-04`, `S1-05`, `S1-08` all merged. The `asan` preset in [CMakePresets.json:49-56](../../CMakePresets.json#L49-L56) now carries `-fsanitize=address,undefined -fno-sanitize-recover=undefined` and the CI job at [.github/workflows/ci.yml:141-162](../../.github/workflows/ci.yml#L141-L162) gates on it, satisfying the NFR-6 dynamic-analysis piece S1-02 scoped.
- The `_base` preset at [CMakePresets.json:10-20](../../CMakePresets.json#L10-L20) now carries `VCPKG_INSTALLED_DIR=build/_vcpkg_installed` as a CMake cache variable, with the ABI-compatibility footnote that justifies sharing the tree across `debug` / `release` / `asan` and explicitly quarantines a future `tsan` preset — matches the S1-08 rationale verbatim.
- M1-01 landed as a plain aggregate per [docs/STYLE.md §2.1](../STYLE.md) and §2.5 (Rule of Zero): [libs/ingest/include/wildframe/ingest/image_job.hpp:39-62](../../libs/ingest/include/wildframe/ingest/image_job.hpp#L39-L62). `Format` is `std::uint8_t`-backed per `performance-enum-size`.
- `wildframe_log` (S0-14) implements the spdlog-idiomatic `logger->info(...)` call shape specified in [docs/STYLE.md §4.5](../STYLE.md) rather than the rejected `WILDFRAME_LOG_*` macro fan-out. The handle surface is at [libs/log/include/wildframe/log/log.hpp:91-126](../../libs/log/include/wildframe/log/log.hpp#L91-L126); the pattern at [libs/log/src/log.cpp:32](../../libs/log/src/log.cpp#L32) matches [docs/STYLE.md §4.4](../STYLE.md) verbatim; the test at [libs/log/tests/log_test.cpp:96-98](../../libs/log/tests/log_test.cpp#L96-L98) pins the emitted line against that pattern.
- S0-19 output-path helpers are DI-shaped per [docs/STYLE.md §2.12](../STYLE.md) (environment boundary) — every helper in [libs/orchestrator/include/wildframe/orchestrator/paths.hpp:26-43](../../libs/orchestrator/include/wildframe/orchestrator/paths.hpp#L26-L43) takes `home` as a parameter and reads no process state. Tests at [libs/orchestrator/tests/paths_test.cpp](../../libs/orchestrator/tests/paths_test.cpp) cover the `ExpandTilde` edge cases individually.
- CI fail-fast ordering in [.github/workflows/ci.yml:125-139](../../.github/workflows/ci.yml#L125-L139) (configure → format-check → build → test) matches S1-05 as landed.
- The Sprint 2 tracer-bullet pivot (#34) re-sequences the backlog to match handoff §18. [docs/BACKLOG.md:219-363](../BACKLOG.md#L219-L363) now hosts TB-01..TB-09 with explicit fold-in notes on M6-01 (into TB-02) and M5-06 (into TB-07), consistent with handoff §18's "a stub is not a TODO" rule.

### Silent deviations from plan

- **`wildframe_log` is an 8th module that is not in the ARCHITECTURE.md dependency graph.** Handoff §10 lists seven modules (six libs + GUI). S0-14 gave the task author latitude ("a `wildframe_log` library **or** inline header in `wildframe_orchestrator`") and the chosen path was the separate library at [libs/log/](../../libs/log/). That choice is defensible and well-executed, but three downstream doc updates did not follow:
  - [docs/ARCHITECTURE.md:27-48](../ARCHITECTURE.md#L27-L48) — the mermaid graph still shows seven nodes; `wildframe_log` is absent, and every other module is now `PUBLIC` or `PRIVATE` linked to `wildframe::log` once it grows real code (the dependency already exists in the CMake graph via `wildframe_log`'s `PUBLIC spdlog::spdlog` link).
  - [docs/ARCHITECTURE.md:89](../ARCHITECTURE.md#L89) — boundary table still credits `wildframe_orchestrator` with owning `spdlog`. That ownership moved to `wildframe_log` with S0-14.
  - [docs/ARCHITECTURE.md:388-390](../ARCHITECTURE.md#L388-L390) — "Every module → spdlog **directly**. The logging sink is a process-global initialized once at startup (S0-14); it is not an orchestrator responsibility." The "directly" is now incorrect; modules go through `wildframe::log` per [docs/STYLE.md §4.5](../STYLE.md). The paragraph that introduces `wildframe_log` does not exist in ARCHITECTURE.md at all.
  Not a code deviation — every piece of code is consistent with `wildframe_log`'s existence — but `docs/ARCHITECTURE.md` is the architectural source of truth referenced by CLAUDE.md §2, and an agent reading it today gets a stale picture.
- **`ImageJob.size_bytes` and `ImageJob.content_hash` ship as public `std::optional<T>` members with zero current callers.** [libs/ingest/include/wildframe/ingest/image_job.hpp:52-61](../../libs/ingest/include/wildframe/ingest/image_job.hpp#L52-L61) documents them as "Stubbed for M1-01 (`std::nullopt`) and populated by a later task when a concrete consumer lands … Do not branch on this field in downstream code until that task lands." That is precisely what [docs/STYLE.md §2.11](../STYLE.md) ("Do not ship speculative interface surface") forbids: exported struct members prepared for a caller that will land in a future task, understood / formatted / tidied / kept-in-sync for zero present value, whose shape often diverges from the prepared one. M1-01's backlog entry at [docs/BACKLOG.md:369-372](../BACKLOG.md#L369-L372) explicitly says "content hash (optional, deferred)." Defer and delete from the struct, or commit to a caller now.
- **`src/CMakeLists.txt` still links the stub GUI against all six static libraries.** [src/CMakeLists.txt:5-13](../../src/CMakeLists.txt#L5-L13). Flagged in the prior review; captured as M7-11 at [docs/BACKLOG.md:715-720](../BACKLOG.md#L715-L720) with "runnable in Sprint 2 once TB-02 lands." Not a regression, not yet closed. Worth noting only so it is not forgotten; the Sprint 2 agent pickup order at [docs/BACKLOG.md:236-247](../BACKLOG.md#L236-L247) correctly schedules it after TB-02.
- **CLAUDE.md §2 "Where to find things" table has drifted behind the docs tree.** [CLAUDE.md:20-34](../../CLAUDE.md#L20-L34) points to `bird_photo_ai_project_handoff.md`, `docs/ARCHITECTURE.md`, `docs/METADATA.md`, `docs/STYLE.md`, `docs/LICENSING.md`, and `docs/BACKLOG.md`. It does **not** list [docs/CONFIG.md](../CONFIG.md) (S0-18), [docs/FIXTURES.md](../FIXTURES.md) (S0-12), [docs/DEV_SETUP.md](../DEV_SETUP.md) (S0-15), or [README.md](../../README.md) (S0-16). An agent following CLAUDE.md §2 to find the TOML config contract, the fixture policy, or the dev-environment bootstrap will not find them. Not a code bug; a discoverability drift since the first docs under Sprint 0 landed.

### Planned but missing

- **Sprint 2 is planned but not started.** #34 landed the plan; no `TB-*` task is `[x]` yet. This is expected and correct — the PR is from today's date. Calling it out only to anchor the next review's baseline.
- **[tests/](../../tests/) remains empty.** Handoff §11 reserves it for cross-module integration tests; TB-09 at [docs/BACKLOG.md:349-363](../BACKLOG.md#L349-L363) will be its first inhabitant. Nothing blocks that; logging the state.
- **`data/config.default.toml` is referenced by [docs/CONFIG.md §1.2](../CONFIG.md) and by S0-19's backlog entry** but the file's current contents were not examined in this review window (it was in scope at the prior review; S0-18 landed then). Not a gap — I chose not to re-audit a file that has not changed. Flag here so the next review knows that check was skipped today.

---

## 2. Dependency Reassessment

Two new buckets of code have landed since the prior review — `wildframe_log` (~427 LoC across header/impl/tests) and the S0-19 path helpers (~125 LoC across header/impl/tests). The prior review's dependency analysis covered the build-system tooling (`tools/fetch_*.cmake`, `ci.yml`) and still stands; this review looks at the two new modules against NFR-6.

### Candidates identified

- **Module:** `wildframe_log`.
- **In-tree code it would replace:** none worth replacing. The module is already a thin adapter over spdlog: [libs/log/src/log.cpp:41-72](../../libs/log/src/log.cpp#L41-L72) (`Init`) is 30 lines of sink construction and logger registration, and the handle type at [libs/log/include/wildframe/log/log.hpp:91-126](../../libs/log/include/wildframe/log/log.hpp#L91-L126) is 35 lines that forward directly to `spdlog::logger::{info,warn,error,critical}`. Every wrap is justified under [docs/STYLE.md §2.9](../STYLE.md) ("wrapping dependency types"): the handle narrows the surface to the four always-on levels and keeps `spdlog::` APIs out of caller code, and the macro pair `WF_TRACE` / `WF_DEBUG` is the one concession that §2.8 permits (compile-time `SPDLOG_ACTIVE_LEVEL` strip genuinely cannot be spelled as a function).
- **Candidate library:** no alternative worth considering. spdlog is the chosen backend (handoff §5, §16 decision 21). Replacing the handle layer with direct `spdlog::get(...)->info(...)` calls would re-introduce the very scatter the handle was added to prevent.
- **Recommendation:** **reject**. Do not swap spdlog; do not remove the handle. Keep.

- **Module:** `wildframe_orchestrator` — path-helper portion.
- **In-tree code it would replace:** [libs/orchestrator/src/paths.cpp:8-48](../../libs/orchestrator/src/paths.cpp#L8-L48). 40 lines of `ExpandTilde` / `DefaultLogPath` / `DefaultManifestDir`, purely `std::filesystem::path` manipulation.
- **Candidate library:** would be some "XDG-aware path helper" — but the helpers here are deliberately not XDG-aware. [docs/CONFIG.md §1.2](../CONFIG.md) pins the macOS fallback explicitly. Boost.Filesystem is already redundant with C++17 `<filesystem>`; Microsoft's `wil::GetUserProfileDirectory` is Windows-only. No macOS-native library offers this with a smaller surface than the 40 lines in-tree.
- **Recommendation:** **reject**. 40 hand-rolled lines, fully tested at [libs/orchestrator/tests/paths_test.cpp](../../libs/orchestrator/tests/paths_test.cpp), are the correct shape.

### Modules reviewed, no candidate worth considering

- `wildframe_ingest`: one public value type, no implementation yet. Nothing to replace.
- `wildframe_raw`, `wildframe_detect`, `wildframe_focus`, `wildframe_metadata`, `wildframe_orchestrator` (non-path portion): still stub umbrella headers. Re-do dependency reassessment as M2-01 / M3-01 / M4-01 / M5-02 / M6-02 land real code.

---

## 3. NFR Drift

| NFR | State | Evidence |
|---|---|---|
| NFR-1 Performance | not yet measurable | No pipeline executable. Stage implementations still stubs; benchmark task I-02 depends on the fully thickened slice. |
| NFR-2 Accuracy | not yet measurable | Fixture corpus still 3 of 5 categories; categories 4 and 5 pinned at [tools/fetch_fixtures.cmake](../../tools/fetch_fixtures.cmake) deferred to S0-12b pending customer CR3 supply. No change since the prior review. |
| NFR-3 Extensibility | on track | [docs/ARCHITECTURE.md §4](../ARCHITECTURE.md) still specifies the `PipelineStage` contract; no code contradicts it yet. TB-02 at [docs/BACKLOG.md:274-284](../BACKLOG.md#L274-L284) is the first task that will publish the abstract interface, carrying M6-01 into Sprint 2. |
| NFR-4 Interoperability | on track | [docs/METADATA.md](../METADATA.md) contract unchanged since prior review. |
| NFR-5 Auditability | **upgraded to operational** (from "on track, pre-code") | `wildframe_log` is live. [libs/log/src/log.cpp:32](../../libs/log/src/log.cpp#L32) implements the exact `%Y-%m-%d %H:%M:%S.%e [%^%l%$] [%n] %v` pattern specified by [docs/STYLE.md §4.4](../STYLE.md); [libs/log/src/log.cpp:49-53](../../libs/log/src/log.cpp#L49-L53) configures `daily_file_sink_mt` with `max_files=14` per [docs/STYLE.md §4.3](../STYLE.md); [libs/log/tests/log_test.cpp:85-106](../../libs/log/tests/log_test.cpp#L85-L106) pins the emitted-line shape against that pattern. Flush-on-warn at [libs/log/src/log.cpp:69](../../libs/log/src/log.cpp#L69) matches [docs/STYLE.md §4.7](../STYLE.md) ("error/critical lines are on disk before the orchestrator writes the corresponding manifest row"). |
| NFR-6 Maintainability (**primary**) | on track | Total in-tree C++ is 734 LoC across 22 files ([find \| wc -l](../../libs/)). Largest module is `wildframe_log` at 166 hpp + 122 cpp = 288 LoC — justified by the spdlog adapter surface, not gold-plated. Every `.cpp` implementation file stays well under NFR-6's ~2000-line guideline. Two maintainability debts to log: (a) the `ImageJob` speculative optional fields flagged in §1; (b) the still-unfixed `src/CMakeLists.txt` over-link, also flagged. Neither is a regression. |
| NFR-7 Code Standards | on track | [docs/STYLE.md §2.1](../STYLE.md) now spells the accessor example as `box.Area()` (CamelCase), closing the prior review's §1 deviation. Exception policy at [docs/STYLE.md §3](../STYLE.md) is mirrored by the `wildframe_log` docstring at [libs/log/include/wildframe/log/log.hpp:135-136](../../libs/log/include/wildframe/log/log.hpp#L135-L136) ("The orchestrator translates that to a Wildframe error type at the boundary"). Format-check gate in CI is active; no drift observed. |
| NFR-8 Static Analysis | **on track, with one scope-tightening opportunity** | Total in-tree `NOLINT` sites: 8, up from 0 at the prior review. Every one has an inline justification that documents what real fix was tried; none is a premature suppression per [docs/STYLE.md §2.10](../STYLE.md). Detail below. The one loose edge is that [libs/log/src/log.cpp:11](../../libs/log/src/log.cpp#L11) is a file-wide `NOLINTBEGIN(misc-include-cleaner)` wrapping 111 lines — STYLE §2.10 prefers "tight `NOLINTBEGIN/END` pair … scoped to the exact line(s)." Worth scope-tightening to the specific include lines the checker flags, or converting to per-line `NOLINT` comments. Not a gate-failure, just a style preference the suppression author already acknowledged in the comment ("rather than leaking NOLINTs onto every line"). |

### clang-tidy suppression audit

- **Total in-code suppressions:** 8 — all in `libs/log/`. Breakdown:
  - [libs/log/src/log.cpp:11, 122](../../libs/log/src/log.cpp#L11-L122) — file-wide `NOLINTBEGIN/NOLINTEND(misc-include-cleaner)`. Justification in-file: `<spdlog/spdlog.h>` is the umbrella header, but `misc-include-cleaner` flags every `spdlog::level::*` / `spdlog::register_logger` reference as needing a direct include. Concrete false positive. **Scope tightening opportunity** — this could be a per-line `// NOLINT(misc-include-cleaner)` on each offending site instead of a file-wide block.
  - [libs/log/tests/log_test.cpp:8](../../libs/log/tests/log_test.cpp#L8) — single-line `NOLINT(misc-include-cleaner)` on `<spdlog/sinks/ostream_sink.h>`. False positive; justified inline.
  - [libs/log/tests/log_test.cpp:102](../../libs/log/tests/log_test.cpp#L102) — `NOLINTNEXTLINE(readability-implicit-bool-conversion)` on a GTest streaming-message expansion. False positive; justified.
  - [libs/log/include/wildframe/log/log.hpp:102-122](../../libs/log/include/wildframe/log/log.hpp#L102-L122) — `NOLINTBEGIN/END(cppcoreguidelines-missing-std-forward)` around the four member-template forwarding sites. In-file comment documents the concrete false positive (the checker does not see through parameter-pack expansion of `std::forward<Args>(args)...`). Scoped to just the member templates.
  - [libs/log/include/wildframe/log/log.hpp:161-166](../../libs/log/include/wildframe/log/log.hpp#L161-L166) — `NOLINTBEGIN/END(cppcoreguidelines-macro-usage)` around `WF_TRACE` / `WF_DEBUG`. Documented as the one kind of macro STYLE §2.8 permits (compile-time level strip); the "real fix" column is explicitly addressed ("A plain function call would always compile and keep trace-level format cost in release binaries").
- **New since last review:** all 8 — first module with real code to suppress against.
- **Warrant re-examination:** [libs/log/src/log.cpp:11](../../libs/log/src/log.cpp#L11)'s file-wide scope (tighten when convenient, not urgent).
- **Global `.clang-tidy` disables:** unchanged from prior review (13 preference-based disables, all still justified inline). The prior review flagged three for re-examination once real code exists — `cppcoreguidelines-pro-bounds-pointer-arithmetic`, `cppcoreguidelines-narrowing-conversions`, `bugprone-easily-swappable-parameters`. None have fired / been needed across the 734 LoC of real code that landed. Re-audit once M1-02/M2-01/M3-02 land pointer-arithmetic- and narrowing-conversion-heavy code.

### Readability spot-checks

Two files picked from the newly-landed code, read with an "average embedded C++ developer, new to Wildframe" lens:

- [libs/log/include/wildframe/log/log.hpp](../../libs/log/include/wildframe/log/log.hpp) — passes. The Doxygen header at lines 1-22 gives a worked example (`log::detect.info(...)`, `WF_TRACE(log::detect, ...)`). The `Config` struct at lines 47-71 documents every field and links each back to its design source (STYLE §4.2, §4.3). The `Logger` type at lines 91-126 is a stateless handle — 35 lines, no cleverness. The only advanced idiom is the `template <class... Args>` forwarding pack, and it is spelled with `spdlog::format_string_t<Args...>` which is already the spdlog-native idiom (satisfies CLAUDE.md §5's "verify the library's idiomatic usage" rule). The `inline constexpr Logger detect{"detect"};` namespace-scope handle pattern at lines 145-151 is unusual but well-chosen and documented: "`inline constexpr` gives external linkage with ODR safety — every translation unit sees the same object." A new reader will not trip.
- [libs/orchestrator/src/paths.cpp](../../libs/orchestrator/src/paths.cpp) — passes. 49 lines. Two constexpr relative paths are spelled out verbatim at [lines 14-16](../../libs/orchestrator/src/paths.cpp#L14-L16) with an inline note ("spelled out here so the resolved absolutes appear verbatim in the tests"). `ExpandTilde` handles `~`, `~/`, `~/<rest>`, `~user/<rest>`, and non-tilde inputs as four explicit branches — readable in one pass. The one subtlety (`home / ""` appending a separator in libc++) is inline-commented at [line 35](../../libs/orchestrator/src/paths.cpp#L35). No templates, no cleverness.

Both files would pass NFR-6 on a code review from an average-skilled embedded C++ developer without follow-up questions.

---

## 4. Risk Register Update

### Risks elevated since last review

- None.

### Risks no longer relevant

- **Prior review §4 "CI first-build cost" (cold-cache timeout on `macos-14`).** Mitigated by S1-01's three-layer caching at [.github/workflows/ci.yml:100-123](../../.github/workflows/ci.yml#L100-L123). The workflow has successfully run post-S1-01 per its own comments ("run 24782789092" referenced at [ci.yml:60](../../.github/workflows/ci.yml#L60) / [BACKLOG S1-08:215](../BACKLOG.md#L215)). Time-to-green has not been re-measured in this review, but the steady-state builds referenced in the S1-04 commentary at [BACKLOG.md:197](../BACKLOG.md#L197) put tidy at "seconds" — dramatically under the 180-minute timeout, now 200 per [ci.yml:53](../../.github/workflows/ci.yml#L53). Keep as a watch item, not a risk.

### New risks discovered

- **Doc/code coupling drift as modules land.** The `wildframe_log` path from S0-14 introduced a module that the architecture doc did not follow into. This is the first instance of a broader risk: as M1/M2/M3 land, each task can introduce architectural decisions (a new module, a new link edge, a relocated responsibility) that docs/ARCHITECTURE.md has to catch up with. CLAUDE.md §5 already forbids silent code-side drift from the handoff doc, but ARCHITECTURE.md is "just" a derived doc — easy to forget. Mitigation: add a checkbox to §7's working loop, or to the PR template, that any module-level structural change updates ARCHITECTURE.md in the same PR.
- **Asset-hosting single point of failure (prior review §4).** Still present. [tools/fetch_models.cmake:67](../../tools/fetch_models.cmake#L67) and [tools/fetch_fixtures.cmake:75-90](../../tools/fetch_fixtures.cmake#L75-L90) both point at `github.com/mroy113/wildframe/releases/`. Unchanged; not elevated, but worth carrying forward.

---

## 5. Backlog Grooming

### Re-order

- None. The Sprint 2 pickup order at [docs/BACKLOG.md:236-247](../BACKLOG.md#L236-L247) is internally consistent with the dependency graph and satisfies handoff §18's tracer-bullet strategy.

### Re-size

- None. M1-01 landed as sized (S); S1-02 landed as sized (S); S0-19 landed as sized (S); S0-14 landed as sized (S) — though it required a mid-task redesign (per [CLAUDE.md §5](../../CLAUDE.md#L86) bullet "Verify the library's idiomatic usage before designing"). That redesign was absorbed inside the task rather than causing an overrun, so no size change is warranted; the lesson has already been codified in CLAUDE.md.

### Add

- **Proposed task ID:** S0-23 (or an M0-prefixed mop-up task — suggest S0-23 to group with the rest of the scaffolding-era closures).
  **Title:** Update `docs/ARCHITECTURE.md` for `wildframe_log` (mermaid graph node, boundary table row, §6 cross-cutting paragraph).
  **Rationale:** §1 "silent deviations" above — `wildframe_log` is an 8th module not reflected in the architecture doc referenced by CLAUDE.md §2. Justified by §1 / §3 findings.
  **Deps:** none (doc-only).
  **Size:** S.
  **Satisfies:** NFR-6 (doc/code coupling — an agent reading ARCHITECTURE.md as source of truth must see reality).

- **Proposed task ID:** S0-24 (doc-only, discoverability).
  **Title:** Update `CLAUDE.md` §2 "Where to find things" table for `docs/CONFIG.md`, `docs/FIXTURES.md`, `docs/DEV_SETUP.md`, `README.md`.
  **Rationale:** §1 "silent deviations" above — agents following CLAUDE.md §2 for TOML config / fixtures / dev-env docs reach a dead end.
  **Deps:** none.
  **Size:** S (one table, five rows).
  **Satisfies:** NFR-6 (discoverability).

- **Proposed decision (not a new task):** decide whether `ImageJob.size_bytes` and `ImageJob.content_hash` should ship now or be removed pending caller.
  **Rationale:** §1 "silent deviations" above — public `std::optional<T>` members with no caller and "do not branch on these" docstrings are speculative interface surface per [docs/STYLE.md §2.11](../STYLE.md). Two viable resolutions: (a) remove both fields from the M1-01 struct and re-introduce them at the first consumer's task; (b) ratify the fields as load-bearing invariants with a §2.11 exception recorded in the struct's Doxygen (e.g. "required now so the manifest writer in TB-08 has the size available for the stage-timing row"). Either is fine; what does not work is leaving the fields with their current "stub — do not read" posture. Flagging as a decision, not a task, because the scope depends on which resolution is chosen.

### Remove or mark obsolete

- None.

---

## 6. Open Ambiguities

- **Is `wildframe_log` a first-class module?** Handoff §10 still lists seven MVP modules. S0-14 permitted the spin-off ("a `wildframe_log` library or inline header"). The chosen path (library) adds an 8th module; the handoff doc has not been amended to reflect that, nor has ARCHITECTURE.md. Customer call on whether to (a) update handoff §10 / ARCHITECTURE.md to formally recognize `wildframe_log` as Module 0 (scaffolding) or Module 8, or (b) keep it informal and have ARCHITECTURE.md describe it as a "cross-cutting utility" rather than a module. Either is defensible; the current state (unrecognized in docs) is not.
- **`ImageJob.size_bytes` / `ImageJob.content_hash` resolution.** See §5 "Add" decision above. §2.11 compliance requires a call.
- **Sprint 2 agent parallelism.** [docs/BACKLOG.md:237-247](../BACKLOG.md#L237-L247) says "parallelizable clusters are grouped" and lists several. Is the intent that a single agent walks the order sequentially (as happens today), or that the customer actively wants multiple concurrent agents picking tasks from the parallel clusters? The answer matters because TB-03/TB-04/TB-05/TB-06 share types (`PreviewImage`, `BBox`, `DetectionResult`, `FocusResult`) across headers that must land in a coherent order — parallelizing requires a shared header-definition-landing strategy that the backlog does not spell out. Customer call on whether to document the strategy explicitly in the Sprint 2 preamble or leave it implicit.
- **First-run CI timing (carried from prior review).** Steady-state latency looks good but the cold-cache number has not been re-measured in this review. A single `gh run view` data point would confirm.

---

## Summary

`main` is in healthy shape and has advanced meaningfully since the 2026-04-21 review. Sprint 0 is closed; the Sprint 1 CI follow-ups that gate future work on dynamic-analysis coverage and build-cache stability have landed; the first piece of module code (`ImageJob`) and the first cross-cutting runtime library (`wildframe_log`) are merged with test coverage and lint discipline intact; the Sprint 2 tracer-bullet plan is in place. 734 LoC of real code across the repo, 8 `NOLINT` suppressions, all justified — NFR-8's "zero findings" gate holds with room to spare.

The two concerns worth addressing before Sprint 2 code starts landing are both doc-coupling issues rather than code bugs: (1) `docs/ARCHITECTURE.md` has not caught up with the `wildframe_log` module, which leaves agents reading it for the architectural source of truth with a stale picture, and (2) the `ImageJob` struct ships two `std::optional` stub fields that read as speculative interface surface per [docs/STYLE.md §2.11](../STYLE.md) — decide to remove-pending-caller or ratify-with-justification before the field names propagate into the TB-07 / TB-08 manifest writer. Neither blocks Sprint 2 from starting; both get cheaper to fix today than after five more tasks take dependencies on the current shape.
