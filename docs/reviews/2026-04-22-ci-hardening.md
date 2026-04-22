# Project Health Review — CI Hardening

## Metadata

- **Milestone:** CI hardening (S0-13 + S1-01 + S1-04 + S1-05 landed; S1-06 filed). This review targets the delta on branch `ci/s0-13-github-actions` since the 2026-04-21 Sprint 0 infrastructure review. `/review` was invoked with an empty milestone argument; customer confirmed "the current branch with the CI work" is the state to assess, so the slug scopes to the CI-hardening slice rather than Sprint 0 as a whole.
- **Date:** 2026-04-22
- **Commit reviewed:** `9540244849764b146d21d68556724f69db01ecf6` (branch `ci/s0-13-github-actions`, not yet merged; `main` last advanced at `a0d3d13`).
- **Reviewer agent:** Claude Opus 4.7, review-only.
- **Scope examined:** [.github/workflows/ci.yml](../../.github/workflows/ci.yml), [CMakePresets.json](../../CMakePresets.json), [CMakeLists.txt](../../CMakeLists.txt), [cmake/PlatformMinimums.cmake](../../cmake/PlatformMinimums.cmake), [cmake/platforms/macos.cmake](../../cmake/platforms/macos.cmake), [cmake/platforms/linux.cmake](../../cmake/platforms/linux.cmake), [cmake/platforms/windows.cmake](../../cmake/platforms/windows.cmake), [tools/bootstrap.sh](../../tools/bootstrap.sh), [docs/DEV_SETUP.md](../DEV_SETUP.md), [docs/BACKLOG.md](../BACKLOG.md) §Sprint 0 / §Sprint 1, and Sprint 0 artifacts the 2026-04-21 review flagged for follow-up ([docs/STYLE.md §2.1](../STYLE.md), [src/CMakeLists.txt](../../src/CMakeLists.txt)). GitHub Actions run history inspected via `gh run list --branch=ci/s0-13-github-actions` and `gh run view --json jobs`. Not examined: any C++ module code (still stubs), per-port vcpkg build logs, x-gha binary cache occupancy.

---

## 1. Plan Fidelity

### Matches plan

- S0-13 contract fully covered in [.github/workflows/ci.yml](../../.github/workflows/ci.yml): configure+build (debug at line 117, release at line 133), `ctest` (line 130), format-check (line 124), and clang-tidy gate (line 147). All on `macos-14`, all wired as required steps under a single job.
- S1-01 (vcpkg_installed cache) landed at [.github/workflows/ci.yml:100-106](../../.github/workflows/ci.yml#L100-L106) with a cache key that includes `vcpkg.json` plus the S0-20 platform-minimums CMake files, matching the task's filed rationale. `restore-keys` allows partial-hit reconciliation on manifest-only changes.
- S1-04 (tidy via `run-clang-tidy -p build/debug`) landed at [.github/workflows/ci.yml:146-153](../../.github/workflows/ci.yml#L146-L153); the dedicated `tidy` preset at [CMakePresets.json:37-45](../../CMakePresets.json#L37-L45) is retained only for the local editor loop, as S1-04's backlog text describes.
- S1-05 (fail-fast format-check) at [.github/workflows/ci.yml:123-124](../../.github/workflows/ci.yml#L123-L124) runs immediately after `Configure (debug)` and before `Build (debug)`.
- S0-15 dev-environment bootstrap at [tools/bootstrap.sh](../../tools/bootstrap.sh) mirrors CI's Homebrew install list and pins vcpkg to the `builtin-baseline` SHA from `vcpkg.json` via `git checkout` — same mechanism CI uses at [.github/workflows/ci.yml:78-89](../../.github/workflows/ci.yml#L78-L89). No drift between dev-env and CI.
- S0-20 platform-minimums gate ([cmake/PlatformMinimums.cmake](../../cmake/PlatformMinimums.cmake) + [cmake/platforms/macos.cmake](../../cmake/platforms/macos.cmake)) sets `CMAKE_OSX_DEPLOYMENT_TARGET=13.0` pre-`project()` and enforces Apple Clang ≥ 15 post-`project()`, exactly per the handoff §16.24. Linux and Windows files are explicit stubs with clear "do not populate without a plan-change PR" instructions.
- Two prior-review (2026-04-21) §5 backlog additions are present: [docs/BACKLOG.md](../BACKLOG.md) S0-22 (LICENSE + vcpkg.json license field) at line 163 and M7-11 (trim GUI link line) at line 546.
- Prior review's §1 silent-deviation on accessor casing is fixed: [docs/STYLE.md:95](../STYLE.md#L95) now spells the example `box.Area()`, consistent with the CamelCase rule one row above.
- S1-06 filed rather than implemented, with an explicit "don't pull the trigger yet" trigger criterion ("wall time crosses ~15 min of actual work") at [docs/BACKLOG.md:205-209](../BACKLOG.md#L205-L209). Matches CLAUDE.md §5's "scope the judgment" rule.

### Silent deviations from plan

- **CI-only install-tree override not mirrored in presets.** [.github/workflows/ci.yml:54](../../.github/workflows/ci.yml#L54) sets `VCPKG_INSTALLED_DIR: ${{ github.workspace }}/build/_vcpkg_installed` as a job-level env var so all three presets share one installed tree. [CMakePresets.json](../../CMakePresets.json) does not set this — local `cmake --preset debug` still lands vcpkg ports in `build/debug/vcpkg_installed/`. Benign for CI correctness (the comment at ci.yml:50-53 correctly notes all three presets use the same `arm64-osx` triplet), but it means the "reuse warm build directories whenever possible" advice in [docs/DEV_SETUP.md §8](../DEV_SETUP.md#L149-L168) does not get the same cross-preset sharing locally that CI gets. Worth considering whether to promote `VCPKG_INSTALLED_DIR` into the presets' cache variables — or at minimum document the divergence — before M1 lands and local iteration matters.
- **Shared-install-tree invariant is implicit.** The comment at [.github/workflows/ci.yml:50-53](../../.github/workflows/ci.yml#L50-L53) documents that sharing works because all presets use the same `arm64-osx` triplet. Nothing in the repo enforces this — a future preset that introduces a different triplet, different `VCPKG_OVERLAY_*` dirs, or different C++ flags that affect ABI would silently poison the shared tree and the cache key. Not actionable today (the invariant holds), but worth a NOTE in [CMakePresets.json](../../CMakePresets.json) so the next contributor sees the trap.
- **`VCPKG_INSTALLED_DIR` is not reflected in the S1-01 cache key.** The cache key at [.github/workflows/ci.yml:104](../../.github/workflows/ci.yml#L104) hashes `vcpkg.json` + `cmake/PlatformMinimums.cmake` + `cmake/platforms/macos.cmake`. A change to `VCPKG_INSTALLED_DIR` that altered the layout or triplet of the installed tree would not invalidate the cache. Again not actionable today, but the key should be revisited if the env var's value ever changes.
- **Timeout raise was not tracked in the backlog.** The 180 → 240 minute bump at [.github/workflows/ci.yml:45](../../.github/workflows/ci.yml#L45) landed in commit `0379c00` ("raise timeout") without a backlog entry. The prior review's §4 flagged exactly this risk, and the bump is the right response, but the chain of reasoning lives only in the commit message. Low severity — would be cleaner if the justification ("cold cache clocked 3h4m36s on run 24755537719, exceeded the original 180m ceiling by 4m") lived in a comment at ci.yml:45 too, so a reader does not have to `git log -p` to find it.

### Planned but missing

- **S1-01's steady-state latency claim is not yet empirically confirmed.** The last *successful* CI run (run ID 24755537719, completed 2026-04-22T04:38:51Z, wall time **3h04m36s**) was at commit `0379c00`, which predates the S1-01 + S1-04 + S1-05 landings in commits `4ff256c` and `e687611`. Its step breakdown shows three ~60-minute configure phases (debug, release, tidy) — i.e. the `tidy` preset still ran. The first post-S1-01 run on this branch (ID 24782789092) was still in progress at 14:06+44m wall time when this review was written, so whether the warm-cache run completes in "~minutes" (as S1-01's filed text claims) or longer remains unobserved. The cache-restore step itself (`Cache vcpkg_installed tree`) completed in ~1 second on that run at [run step 6], indicating either a cold miss or a nearly-empty cache — diagnosing which is outside this review's scope but is the single most important open data point for the milestone. Customer should confirm at least one green warm-cache run before declaring the CI-hardening milestone done.
- **S0-13 claims "Cache vcpkg and `build/_models/`"** in [docs/BACKLOG.md:102](../BACKLOG.md#L102). The workflow caches both ([.github/workflows/ci.yml:108-114](../../.github/workflows/ci.yml#L108-L114) covers `build/_models` and `build/_fixtures`; S1-01 covers the `_vcpkg_installed` tree; x-gha covers the vcpkg per-port binary cache). No gap.
- **Remaining Sprint 0 tasks unchanged from prior review:** S0-12b (blocked on customer), S0-14 (spdlog init), S0-16 (README), S0-19 (output path impl), S0-21 (re-analysis behavior). All correctly `[ ]` and out of this milestone's scope.

---

## 2. Dependency Reassessment

No new in-tree code since the 2026-04-21 review — all module `.cpp` files remain stub-sized. CI and bootstrap infrastructure added is shell/YAML, not C++. Default recommendation per NFR-6 is **reject** any swap.

### Candidates identified

- **Module:** CI (workflow YAML).
- **In-tree code it would replace:** `Export GitHub Actions cache env for vcpkg` at [.github/workflows/ci.yml:65-70](../../.github/workflows/ci.yml#L65-L70) (5 lines of inline JS inside a `github-script` action) plus the hand-rolled `Bootstrap vcpkg at vcpkg.json builtin-baseline` step at [ci.yml:78-89](../../.github/workflows/ci.yml#L78-L89) (12 shell lines).
- **Candidate action:** `lukka/run-vcpkg@v11`.
- **Last release / activity:** actively maintained, v11 is current major.
- **Pros vs in-house:** single `uses:` block replaces the env-export shim and the vcpkg clone+checkout+bootstrap pair. Knows how to engage the x-gha backend correctly.
- **Cons vs in-house:** the three hand-rolled steps total 17 lines, each of which is clearly legible to "an average-skilled embedded C++ developer per NFR-6" without reading third-party action documentation. Adopting `lukka/run-vcpkg` trades legibility for brevity that isn't really there. Also introduces a supply-chain dependency in CI.
- **Recommendation:** **reject**, same as 2026-04-21. The prior review reached the same conclusion against a simpler workflow; the workflow is now slightly longer (cache steps, GH-Actions env export) but each addition pays for itself. Re-evaluate if a future task needs vcpkg to do something materially more complex (e.g. cross-compile triplet matrix under S1-06 or P2-01).

- **Module:** CI (cache orchestration).
- **In-tree code it would replace:** the `actions/cache@v4` blocks at [.github/workflows/ci.yml:100-114](../../.github/workflows/ci.yml#L100-L114), 15 lines total across `vcpkg_installed` and the fetched-assets cache.
- **Candidate:** `actions/cache/restore@v4` + `actions/cache/save@v4` split.
- **Pros vs in-house:** gives finer control over save-always-even-on-failure semantics; the unified `actions/cache@v4` skips the save on job failure, which means a transient `cmake --preset release` failure after a successful configure discards the vcpkg tree it just built.
- **Cons vs in-house:** adds a `post:` save step per cache entry, roughly doubling the line count and the reader load. The S1-01 text explicitly chose `actions/cache@v4`'s unified form; no observed pain today justifies splitting.
- **Recommendation:** **reject** for now. File a follow-up only if a run is observed where the vcpkg tree was built successfully and then a downstream step failed — at that point the cost of re-building on the next run is measurable.

### Modules reviewed, no candidate worth considering

- `cmake/platforms/*.cmake` dispatcher: 53 lines of straight-line CMake, no third-party idiom exists that would be clearer.
- [tools/bootstrap.sh](../../tools/bootstrap.sh): 224 lines of shell with strict `set -euo pipefail`, stepwise logging, and explicit rationale headers per section. No third-party replacement — `brew bundle` could eliminate the per-package loop but at the cost of an extra Brewfile to maintain; net-neutral at best for NFR-6.
- All six `wildframe_<module>` libs: unchanged since 2026-04-21 review (still empty stubs).

---

## 3. NFR Drift

| NFR | State | Evidence |
|---|---|---|
| NFR-1 Performance | not yet measurable | No pipeline exists; benchmark task I-02 gates on M6. |
| NFR-2 Accuracy | not yet measurable | Fixture corpus still partial — S0-12b deferred per customer. No change from 2026-04-21. |
| NFR-3 Extensibility | on track | No code added that contradicts the `PipelineStage` contract in [docs/ARCHITECTURE.md](../ARCHITECTURE.md). |
| NFR-4 Interoperability | on track | [docs/METADATA.md](../METADATA.md) unchanged; still comprehensive. |
| NFR-5 Auditability | on track | Logging policy ([docs/STYLE.md §4](../STYLE.md)) is no longer a TODO — landed via commit `0af34f7` under S0-17. Global logger init still gated on S0-14. |
| NFR-6 Maintainability (**primary**) | on track | CI workflow at [.github/workflows/ci.yml](../../.github/workflows/ci.yml) is heavily commented: a 24-line header block explains the three-layer caching strategy, each cache block has a per-key rationale, S1-04/S1-05 blocks explain *why* the step ran where it did. [cmake/PlatformMinimums.cmake](../../cmake/PlatformMinimums.cmake) is 53 lines with one conditional branch per host/phase — no cleverness. [tools/bootstrap.sh](../../tools/bootstrap.sh) has numbered section headers and a per-tool "Why each one" comment block at lines 39-53. An embedded C++ developer arriving cold could read any one of these files in one sitting. One small concern: the `VCPKG_INSTALLED_DIR` local/CI divergence noted in §1 means a local `cmake --preset debug` then `--preset release` sequence has to re-install vcpkg ports for each preset unless the developer also sets the env var, which is not documented in [docs/DEV_SETUP.md](../DEV_SETUP.md). |
| NFR-7 Code Standards | on track | [docs/STYLE.md:95](../STYLE.md#L95) accessor casing fixed per 2026-04-21 §1. Format-check gate wired at [.github/workflows/ci.yml:124](../../.github/workflows/ci.yml#L124) with fail-fast ordering per S1-05. No C++ code exists to fail format-check against, but the gate runs and passes on the empty `libs/*/src/*.cpp` stubs. |
| NFR-8 Static Analysis | on track | Clang-tidy gate at [.github/workflows/ci.yml:146-153](../../.github/workflows/ci.yml#L146-L153) uses `run-clang-tidy -p build/debug` per S1-04, with `-quiet` and the set-exit-on-error shell preamble. Successful run 24755537719 reported no findings (step completed in ~1 second against empty stubs). Check set unchanged from 2026-04-21; 13 preference-based global disables still present, all justified inline. |

### clang-tidy suppression audit

- In-code suppressions (`// NOLINT` / `NOLINTNEXTLINE` / `NOLINTBEGIN`): **0** — unchanged from 2026-04-21. No C++ code exists to suppress against.
- Global disables in [.clang-tidy](../../.clang-tidy): unchanged — 25 duplicate-alias disables (mechanical) + 13 preference-based disables (justified inline).
- New since last review: **0**. Re-audit of the 13 preference-based disables still deferred to "after M1 lands."
- Warrant re-examination: same three flagged on 2026-04-21 — `cppcoreguidelines-pro-bounds-pointer-arithmetic`, `cppcoreguidelines-narrowing-conversions`, `bugprone-easily-swappable-parameters`.

### Readability spot-checks

- [.github/workflows/ci.yml](../../.github/workflows/ci.yml) (153 lines): a new contributor can read the file top-to-bottom, understand the three-layer cache strategy from the header comment, and trace the order of gates (configure → format → build → test → release → tidy). Every non-obvious choice has a *why*: the env-export shim at line 65-70 explains that x-gha reads env vars that need re-export, the install-dir share at line 50-54 explains the triplet invariant, the S1-04 block at line 138-145 explains why `run-clang-tidy` uses debug's compile-commands.
- [cmake/PlatformMinimums.cmake](../../cmake/PlatformMinimums.cmake) (53 lines): dispatcher with three branches, each branch one include. The `WILDFRAME_PLATFORM_PHASE` two-phase pattern is unusual but the file opens with a comment block explaining exactly why — pre-`project()` must set `CMAKE_OSX_DEPLOYMENT_TARGET` before compiler probes, post-`project()` must gate on `CMAKE_CXX_COMPILER_VERSION` which `project()` populates. An embedded dev unfamiliar with the pre/post split could follow it.
- [tools/bootstrap.sh](../../tools/bootstrap.sh) (224 lines): numbered sections, per-tool rationale, strict error handling. The `baseline_sha` parse at lines 174-178 is the most regex-y bit but has a fail path pointing at `vcpkg.json` if the regex breaks.

No NFR-6 violations spotted in the CI-hardening code surface.

---

## 4. Risk Register Update

### Risks elevated since last review

- **Cold-cache CI wall time vs timeout ceiling.** The 2026-04-21 review §4 predicted this: "confirm first CI run actually completes within 3 hours before S0-13 is declared green." The first successful run (24755537719) clocked **3h04m36s** — 184 minutes, 4 minutes over the original 180-minute ceiling. Commit `0379c00` subsequently raised [.github/workflows/ci.yml:45](../../.github/workflows/ci.yml#L45) to 240 minutes, which preempted the failure mode. Observation that elevates this from "new risk" to "active risk": the 3h04m run was at commit `0379c00`, so the subsequent S1-01 cache landing is expected to compress steady-state dramatically — but a future cold-cache rotation (e.g. a macOS runner image refresh that breaks the S0-20 hash key at `ci.yml:104`) still re-triggers the 3h-ish cold path. If a future runner-image cycle pushes the cold path past 240 minutes, the timeout will need another raise or the `macos-14` image will need to be re-pinned.

### New risks discovered

- **S1-01 warm-cache payoff is unverified at review time.** See §1 "Planned but missing." The filed S1-01 text claims "~minutes" steady-state. The first in-flight run post-S1-01 was at 44+ minutes of wall time when this review was written, and the `Cache vcpkg_installed tree` step completed in ~1 second — consistent with either a cold miss (first key hit on this branch) or a trivial hit against an empty cache blob. This should resolve naturally on the next push to the branch, but until one warm run is green, the milestone's central premise is a prediction, not a measurement.
- **Single-job bottleneck means any flake replays the entire ~45-min cold path.** Because CI is a single `macos-14` job, a transient x-gha network blip during `Configure (debug)` at [ci.yml:117](../../.github/workflows/ci.yml#L117) discards all subsequent parallel work. S1-06 is filed for this but deliberately deferred until wall time crosses ~15 min of actual project work. Acceptable today given the backlog shape, but worth revisiting if flakes surface before M1 compiles in meaningful time.
- **Shared `VCPKG_INSTALLED_DIR` invariant has no compile-time enforcement.** See §1 "silent deviations." If anyone (human or agent) adds a preset with a different triplet or ABI-affecting flag, the shared tree silently breaks. Risk is low pre-M1 because no new presets are planned, but elevated once S1-02 / S1-03 sanitizer presets land — sanitizer flags change ABI of every vcpkg-built dependency.
- **Autotools set on Homebrew has a hidden dependency footprint.** [.github/workflows/ci.yml:76](../../.github/workflows/ci.yml#L76) and [tools/bootstrap.sh:54-63](../../tools/bootstrap.sh#L54-L63) install `autoconf autoconf-archive automake libtool pkg-config` for vcpkg's autotools-/meson-based ports. A future vcpkg baseline bump could introduce a transitive port that needs a tool not in this list (e.g. `gettext` for an i18n port, `xz` for an xz-compressed source). The symptom is a configure-time build failure deep inside a port. Known-good mitigation: add new tools to both files in the same PR as any baseline bump that touches this class of port.

### Risks no longer relevant

- 2026-04-21 §6 "vcpkg pin strategy under S0-20" ambiguity. Resolved by S0-15 landing at commit `358b6cc`: [docs/DEV_SETUP.md §4](../DEV_SETUP.md#L65-L80) now explicitly names `builtin-baseline` as "the authoritative vcpkg pin" and requires the local clone to track it. Bootstrap script enforces this at [tools/bootstrap.sh:174-183](../../tools/bootstrap.sh#L174-L183). CI enforces the same at [.github/workflows/ci.yml:78-89](../../.github/workflows/ci.yml#L78-L89).
- 2026-04-21 §6 "`docs/STYLE.md §2.1` accessor casing" ambiguity. Resolved at [docs/STYLE.md:95](../STYLE.md#L95).

---

## 5. Backlog Grooming

### Re-order

- None. Sprint 1's order — S1-01 (done) → S1-02 (gated on M1-05) → S1-03 (gated on M6-03) → S1-04 (done) → S1-05 (done) → S1-06 (deferred) — is internally consistent with the stated dependencies.

### Re-size

- None with evidence. S1-01 was filed as S and landed in the S0-13 PR alongside S1-04/S1-05 — consistent with S sizing even with the co-land.

### Add

- **Proposed task ID:** S1-07.
  **Title:** Measure and document post-S1-01 steady-state CI wall time.
  **Rationale:** S1-01's backlog text claims a "~minutes" steady-state wall time after cache warm-up, but at review time no warm-cache run had yet completed green (see §1 "Planned but missing" and §4 "New risks"). Without one measured data point, S1-06's deferral criterion ("revisit once sequential build(debug) + build(release) + run-clang-tidy wall time crosses ~15 min of actual work") cannot be evaluated. Deliverable: one observed warm-cache wall time and per-step breakdown recorded in an inline comment near [.github/workflows/ci.yml:38-40](../../.github/workflows/ci.yml#L38-L40). Size: XS. Satisfies: NFR-6 (CI latency verifiability). Deps: S1-01.

- **Proposed task ID:** S1-08.
  **Title:** Promote `VCPKG_INSTALLED_DIR` into CMakePresets.json cache variables.
  **Rationale:** §1 silent deviation: CI shares one vcpkg install tree across debug/release/tidy via a job-level env var at [.github/workflows/ci.yml:54](../../.github/workflows/ci.yml#L54), but local `cmake --preset` invocations have no equivalent, so local iteration pays the per-preset install cost CI avoids. Once M1 code lands and developers iterate between debug and release, the divergence becomes a daily friction. Promote the env var into presets' `cacheVariables` or `environment` block so local and CI behave identically — then delete the ci.yml env entry. Alternative: document the env var in [docs/DEV_SETUP.md §7](../DEV_SETUP.md) as a developer convention. Size: S. Satisfies: NFR-6 (ergonomics, local/CI parity). Deps: S0-04.
  **Status update (2026-04-22, post-review):** Absorbed into the parent CI PR as commit `81d553b` after run `24782789092` confirmed the divergence was not merely cosmetic — CMake used per-preset `build/{debug,release}/vcpkg_installed/` because vcpkg's toolchain integration reads `VCPKG_INSTALLED_DIR` from the CMake cache, not the process environment, so the ci.yml env var was silently ignored and the S1-01 cache step saved nothing (`gh cache list` showed no `vcpkg-installed-macos14-*` entry, and the Actions post-step warned `Path Validation Error: Path(s) specified in the action for caching do(es) not exist`). Fix: move `VCPKG_INSTALLED_DIR` into [CMakePresets.json](../../CMakePresets.json) `_base.cacheVariables`, remove the env var from [.github/workflows/ci.yml](../../.github/workflows/ci.yml), leave a comment at the former env-var site pointing readers at the preset. This promotes S1-08 from a §5 proposal to a §1 critical silent-deviation fix; the review artifact retains it as an `Add` entry for audit continuity.

### Remove or mark obsolete

- None.

---

## 6. Open Ambiguities

- **Warm-cache latency is unmeasured.** See §1 / §4 / S1-07 proposal. What is the wall time of a push that hits `build/_vcpkg_installed` from the same cache key? Needs one green run on this branch with a key-match hit to resolve. Until then the milestone's central claim is a prediction.
- **Triplet / ABI invariant for shared `VCPKG_INSTALLED_DIR`.** See §1 / §4. Is there a place in [CMakePresets.json](../../CMakePresets.json) or [CONTRIBUTING.md](../../CONTRIBUTING.md) where a future contributor adding a preset will be warned that their preset must use `arm64-osx` and no ABI-affecting flags, or else the shared install tree will quietly break? Today, the warning lives only in a comment at [.github/workflows/ci.yml:50-54](../../.github/workflows/ci.yml#L50-L54) — one level of indirection away from the CMake files a preset author would edit.
- **S1-02 / S1-03 sanitizer interaction with shared install tree.** See §4 "new risks." Sanitizer builds typically need sanitizer-instrumented dependencies. When S1-02 lands, will it need its own `build/_vcpkg_installed_asan/` (separate cache key, separate tree), or will it share the uninstrumented tree and only instrument Wildframe code? This is an S1-02-time decision, not a review-time decision, but flagging it now prevents the author from discovering the ABI issue mid-task.
- **Runner-image refresh policy.** The cache key at [.github/workflows/ci.yml:104](../../.github/workflows/ci.yml#L104) includes the literal string `macos14`, and the comment at lines 96-97 says "we accept monthly runner-image refreshes within the macos-14 family in exchange for avoiding a monthly 3h cold rebuild." Who decides when to bump `macos-14` → `macos-15`? Is there a trigger (Apple releases Xcode 16, new libc++ ABI, etc.) that should land in [docs/BACKLOG.md](../BACKLOG.md) as a conditional task?

---

## Summary

The CI-hardening milestone delivers the full S0-13 contract plus three of Sprint 1's CI follow-ups (S1-01, S1-04, S1-05) in one merge window, with S1-06 filed as a clearly-bounded watch-item. Every task's backlog text is now traceable to a specific workflow step, every workflow step has a *why* comment, and two prior-review grooming items (S0-22, M7-11) were filed to the backlog as proposed. Prior-review flags on cold-cache timeout (now 240 min) and accessor-casing (now `box.Area()`) are resolved.

The single unresolved concern is empirical: the **warm-cache steady-state latency S1-01 claims has not yet been observed in a green run**. The most recent successful CI run (3h04m36s) predates S1-01's landing, and the first post-S1-01 run is still in flight at review time. Before declaring this milestone closed, the customer should verify that one warm-cache run completes in the "~minutes" band S1-01 predicts; if it doesn't, the S1-06 deferral criterion needs to be re-evaluated against observed reality rather than the filed assumption. Secondarily, the `VCPKG_INSTALLED_DIR` divergence between CI and local presets (S1-08 proposed) should be resolved before M1 module work begins so developers and CI see the same per-preset install semantics.
