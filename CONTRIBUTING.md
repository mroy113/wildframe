# Contributing to Wildframe

Applies to human contributors and AI coding agents alike.

---

## Branching

- `main` is protected. All work lands via PR.
- Branch naming: `<type>/<module-or-area>-<short-desc>`
  - `feat/ingest-directory-enumeration`
  - `fix/focus-edge-clipping-penalty`
  - `chore/vcpkg-bump-onnxruntime`
  - `docs/metadata-namespace-reference`
- One branch per backlog task. Don't bundle unrelated changes.

---

## Commits

Conventional Commits — machine-parseable and enforced by CI.

```
<type>(<scope>): <subject>

[optional body]

[optional footer: Closes #N, Refs FR-3, etc.]
```

Types: `feat`, `fix`, `refactor`, `chore`, `docs`, `test`, `build`, `ci`, `perf`.

Scopes are module names: `ingest`, `raw`, `detect`, `focus`, `metadata`, `orchestrator`, `gui`, or `build`/`docs`/`ci` for cross-cutting work.

Examples:

```
feat(detect): add MegaDetector alternative via config flag

Refs FR-3, NFR-3.
```

```
fix(focus): clamp Laplacian variance to [0,1] before keeper score

Refs FR-4. Previously could emit values >1 when crop had saturated highlights.
```

---

## Pull requests

Every PR must include:

1. **Task ID** from `docs/BACKLOG.md` in the title (e.g. `[M3-04] YOLOv11 postprocessing`).
2. **FR/NFR citations** from `bird_photo_ai_project_handoff.md` in the description.
3. **Summary of behavior change** in one or two bullets — what the user or caller will now see or do.
4. **Test plan** — how the change was verified beyond unit tests, if applicable.

### CI gates (all must pass before merge)

- Build: debug + release presets.
- Tests: `ctest` full suite.
- Format: clang-format clean.
- Static analysis: `clang-tidy` zero findings, no new unexplained suppressions.
- No committed model weights, no committed build artifacts.

### Merge policy

- **Squash merge only.** The main branch history is one commit per merged PR.
- Author does not self-approve. For solo-dev + agent workflows, the customer is the reviewer; CI gates are the non-negotiable floor.
- Force-push to `main` is forbidden. Force-push to feature branches is allowed up until PR is opened for review.

---

## Code review focus

Reviewers should check, in order:

1. Does it satisfy the cited FR/NFR?
2. Is it within module boundaries? (No sneaky cross-module edits.)
3. Does it follow `docs/STYLE.md` — especially the exception policy and "no advanced C++ features without justification"?
4. Would an average-skilled embedded C++ developer understand it without explanation? (NFR-6.)
5. Are public headers documented?
6. Are tests meaningful, or just coverage theater?

Style-only nits on implementation details are lower priority than correctness and maintainability.

---

## Versioning and releases

- Wildframe follows **semver**: `v<MAJOR>.<MINOR>.<PATCH>`.
- Pre-1.0 (current phase): **breaking changes are allowed in minor bumps**. Patch bumps remain backward-compatible.
- Releases are cut by creating an annotated git tag: `git tag -a v0.1.0 -m "…"` on `main`.
- A `CHANGELOG.md` entry is required for every tagged release. Keep entries terse and user-facing; development minutiae belong in commit history, not changelog.
- Model weights, pipeline logic, and XMP schema each have their *own* version fields recorded as provenance per image (see handoff §13) — these are independent of the application version.

---

## Plan change proposals

The handoff document (`bird_photo_ai_project_handoff.md`) is the source of truth, but it is not infallible. If, while working a task, you find a materially better approach than what the plan prescribes, raise it — don't silently deviate.

**Two-PR pattern — required:**

1. **Plan PR first.** Open a PR that modifies only `bird_photo_ai_project_handoff.md` (and, if affected, `docs/BACKLOG.md`). Title: `plan: <short description>`. Body must cover:
   - **What the plan says today** (quote the relevant passage).
   - **What you propose to change it to.**
   - **Why** — tied to the FR/NFR the change better serves. "Cleaner" is not a reason; "reduces maintenance surface per NFR-6 because …" is.
   - **Blast radius** — which modules, tasks, or prior decisions are affected.
   - **Alternatives considered and rejected.**
2. **Implementation PR second.** Only after the plan PR merges. Cite the merged plan PR in the description.

**Scope guard:** plan-change proposals are about *how* to achieve what the plan already says to achieve. Expanding scope ("we should also build X") is a separate backlog addition, not a plan change — open a backlog task with justification instead.

**Never** modify the handoff doc inside an implementation PR to paper over a deviation. That's the one thing this process exists to prevent.

---

## Dependencies

Net-new third-party dependencies require explicit customer approval. To propose one:

1. Open an issue titled `deps: proposal for <library>`.
2. Describe the need, alternatives considered, licensing, vcpkg availability, and clang-tidy cleanliness.
3. Wait for approval before adding to `vcpkg.json`.

Version bumps of existing dependencies are normal PRs (`chore(build): bump onnxruntime 1.x → 1.y`).

---

## What not to do

- Do not push directly to `main`.
- Do not bypass CI (`--no-verify`, suppressing CI workflows, or similar).
- Do not commit: model weights, `build/`, IDE settings, `.DS_Store`, secrets, or anything under `~/` outside the repo.
- Do not modify `bird_photo_ai_project_handoff.md` to match code reality — flag divergence to the customer instead.
- Do not introduce silent behavior changes. If behavior changes, say so in the PR.
