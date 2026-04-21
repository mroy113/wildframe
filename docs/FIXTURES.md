# Wildframe Test Fixtures

This document is the **contract** for Wildframe's CR3 test-fixture
corpus: how it is stored, what each fixture exists to exercise, and how
to add or replace one. It is the resolution record for the S0-12
customer decision ("Git LFS vs fetch at test-setup time").

Fixtures are the ground truth that module tests key on. Swapping or
removing a fixture changes what the tests prove, so every pin is
versioned and justified below.

This document is the companion to handoff §NFR-2 (accuracy). Where the
two disagree, the **handoff doc wins** — flag the drift per
`CLAUDE.md` §5 rather than editing either file to match the other.

**Scope.** MVP only. Phase 2+ fixtures (species-identification, action-
tag) will append to §2 when those modules land.

---

## 1. Storage policy

**Decision (S0-12):** fixtures are **not committed**. They are fetched
at configure time from a pinned GitHub Release, mirroring the S0-11
model-weights pattern.

### 1.1 Why fetch, not Git LFS

| Criterion | Fetch (chosen) | Git LFS (rejected) |
|---|---|---|
| Repo clone size | Unchanged | Balloons with every fixture revision |
| Bandwidth cost | GitHub Release bandwidth (generous, public) | Counts against LFS quota per account |
| Pattern consistency | Matches S0-11 `fetch_models.cmake` | Introduces a new mechanism |
| Dev setup friction | `cmake --preset debug` pulls everything | Adds `git lfs install` step |
| Offline re-clone | First fetch requires network; cached after | Same after `git lfs pull` |
| Pin discipline | URL + SHA256 in `fetch_fixtures.cmake` | LFS pointer + server trust |

The tiebreaker is pattern consistency. Wildframe already fetches large
binary build-time assets (ONNX models) via [tools/fetch_models.cmake](../tools/fetch_models.cmake);
fixtures use the same idiom so contributors learn one mechanism, not two.

### 1.2 Destination

Fetched fixtures land in `build/_fixtures/<filename>`, gitignored and
parallel to `build/_models/`. The directory is created at configure
time and re-used across builds.

### 1.3 Hosting

GitHub Releases on [mroy113/wildframe](https://github.com/mroy113/wildframe/releases),
tagged `fixtures-vN`. The repo is public, so fixtures are publicly
downloadable; provide only fixtures you are willing to publish under
that constraint.

The current pin is `fixtures-v1`.

### 1.4 Pin integrity

Each fixture is pinned by URL **and** SHA256 in
`tools/fetch_fixtures.cmake`. At configure time CMake verifies the
downloaded file's hash against the pin; a mismatch is `FATAL_ERROR`.
Idempotent on re-run — a cache hit skips the network entirely.

Rotating a fixture requires cutting a new `fixtures-vN+1` release and
updating both the URL and SHA256 in the same commit. A stale URL with
a fresh hash (or vice versa) is a bug.

---

## 2. Fixture catalog

Every fixture exists to exercise a specific test assertion. The
"Intent" column is the contract: if a pipeline change breaks that
assertion, the test has caught a real regression.

Fixtures are Canon CR3 files from a Canon EOS R7. Sizes exceed the
S0-12 aspirational ≤10 MB target because CR3 at that body's pixel
count is 40–60 MB before compression; the constraint was "if possible"
and is not achievable with this camera.

| # | Filename | Size | Intent | Dependent tests |
|---|---|---|---|---|
| 01 | `fixture_01_clear_bird.CR3` | 56 MB | Golden-path: sharp single bird, ~10–30% of frame, unambiguous primary subject. Anchor for the "sharp beats blurred" keeper-score assertion. | M1-05, M2-04, M3-07, M4-07, M5-04, M6-07 |
| 02 | `fixture_02_no_bird.CR3` | 45 MB | Negative path: no animals in frame. `bird_present` must be false; focus stage must be skipped; keeper_score ≈ 0. | M1-05, M2-04, M3-07, M4-07, M5-04, M6-07 |
| 03 | `fixture_03_small_distant_bird.CR3` | 42 MB | Exercises the `raw.decode_crop` fallback path: bird occupies < ~2% of frame, preview resolution is insufficient for focus scoring, full-resolution crop must be decoded. | M2-04 (fallback branch), M4-07, M6-07 |

Each fixture's `Make`/`Model`/`FocalLength`/etc. are real EXIF from
the source exposure, so they also serve as round-trip inputs for
M5-04 (EXIF read round-trip).

---

## 3. How to use fixtures from a test

Tests resolve the fixtures directory from the CMake variable
`WILDFRAME_FIXTURES_DIR` defined in `tools/fetch_fixtures.cmake`. The
test build should surface this as a compile-definition or a generated
header so C++ tests can locate fixtures without assuming a working
directory.

Until the first test target lands (M1-05), the precise wiring is
deferred. The invariant tests can rely on: after a successful
configure, `${CMAKE_SOURCE_DIR}/build/_fixtures/<filename>` exists and
has the SHA256 pinned in `tools/fetch_fixtures.cmake`.

---

## 4. Pending fixtures

Two categories from the S0-12 spec are not yet acquired:

- **fixture_04 motion-blurred bird.** Needs a CR3 where the bird is
  large enough to detect but clearly motion-blurred (wing-flap or
  panning failure), so Laplacian variance and FFT high-frequency
  energy are both low. Required by M4-07's "sharp strictly beats
  blurred" assertion.
- **fixture_05 false-positive magnet.** Needs a CR3 of a bird-shaped
  non-bird (statue, sign, decoy, carving) that a COCO-trained YOLO
  could plausibly classify as "bird". Required by M3-06's comparison
  between YOLO and MegaDetector false-positive behavior.

These fixtures are the customer's responsibility to acquire (per the
S0-12 customer-decision carve-out). Dependent tests must detect their
absence and skip with a diagnostic pointing back to this section
rather than failing the build — the tests are not valid without the
fixtures, and `WILDFRAME_FETCH_FIXTURES` should not be a hard
dependency of a green `ctest` until all five land.

When acquired, they should be:

1. Uploaded as additional assets to the `fixtures-v1` release (or a
   new `fixtures-vN` if anything else changes at the same time).
2. Pinned in `tools/fetch_fixtures.cmake` with URL + SHA256.
3. Documented in §2 above with intent and dependent-test list.
4. Referenced from the skip-gate in the dependent tests so the skip
   becomes a run once the fixture is present.

This section is updated (not appended to) when those fixtures land.

---

## 5. Adding or replacing a fixture

1. Confirm the fixture is CR3 (magic `ftyp crx`) and that you are
   willing to publish it on a public GitHub Release.
2. Compute its SHA256: `shasum -a 256 <file>`.
3. Upload as an asset to the current `fixtures-vN` release. If the
   corpus semantics change (intent of an existing fixture shifts,
   camera body changes and affects EXIF round-trip), cut a new
   `fixtures-vN+1` release and rotate every pin.
4. Add or replace the `wildframe_fetch_fixture(...)` call in
   `tools/fetch_fixtures.cmake` with both the new URL and the new
   SHA256 in the same commit.
5. Update §2 above with the filename, size, intent, and dependent-test
   list.
6. Run `cmake --preset debug` from a clean `build/` to verify the
   fetch succeeds and the hash pin matches.

---

## 6. Replacing the whole corpus

Treat a full corpus rotation like a schema migration: justify it in
the PR description, cite the FR/NFR it better serves, and cut a new
`fixtures-vN+1` release with every fixture re-uploaded. Do not mix
corpus rotation with unrelated code changes — the diff should be
auditable as "old corpus → new corpus" by a reviewer comparing
`tools/fetch_fixtures.cmake` and this document.
