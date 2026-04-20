# CLAUDE.md — Wildframe Operational Manual

This file is the first thing any AI coding agent should read when working in this repo.

---

## 1. What Wildframe is

C++ desktop application that analyzes folders of RAW bird photographs and writes metadata (detection, focus, keeper score) to XMP sidecars. See `bird_photo_ai_project_handoff.md` for full scope, architecture, and decisions.

**Source of truth for scope and decisions: `bird_photo_ai_project_handoff.md`.** If a task seems to conflict with that document, **stop and flag** — do not make the change.

---

## 2. Where to find things

| You need to know… | Read… |
|---|---|
| What the project is and why | `bird_photo_ai_project_handoff.md` §1–§3 |
| What's in scope for MVP | `bird_photo_ai_project_handoff.md` §4 |
| Functional requirements (FR-1..FR-8) | `bird_photo_ai_project_handoff.md` §8 |
| Non-functional requirements (NFR-1..NFR-8) | `bird_photo_ai_project_handoff.md` §9 |
| Module responsibilities and boundaries | `bird_photo_ai_project_handoff.md` §10 + `docs/ARCHITECTURE.md` |
| Repo layout | `bird_photo_ai_project_handoff.md` §11 |
| XMP field names and types | `docs/METADATA.md` |
| Code style, exception policy, Core Guidelines vs Google conflicts | `docs/STYLE.md` |
| Qt LGPL and model-weight licensing | `docs/LICENSING.md` |
| What to work on next | `docs/BACKLOG.md` |
| Branching, PR, commit conventions | `CONTRIBUTING.md` |

---

## 3. Commands

Assume `cmake --preset <name>` from repo root. Presets are defined in `CMakePresets.json`.

```bash
# Configure (first time, or after vcpkg.json changes)
cmake --preset debug

# Build
cmake --build --preset debug

# Run all tests
ctest --preset debug

# Static analysis gate (must pass with zero findings)
cmake --build --preset tidy

# Format check
cmake --build --preset debug --target format-check

# Auto-format (local use only)
cmake --build --preset debug --target format
```

If a preset is missing or a command fails, the repo scaffolding is incomplete — check `docs/BACKLOG.md` for the current Sprint 0 state.

---

## 4. Definition of Done (applies to every task)

A task is not done until **all** of these hold:

1. Code compiles in both `debug` and `release` presets.
2. `ctest --preset debug` passes, including new tests for any new code.
3. `cmake --build --preset tidy` produces **zero clang-tidy findings**. No new `NOLINT` suppressions without a justification comment citing either a confirmed false positive or a considered alternative.
4. `format-check` passes.
5. Public headers have brief Doxygen comments on their public interfaces.
6. The task's PR description cites the FR-x and/or NFR-x it satisfies, from `bird_photo_ai_project_handoff.md`.
7. For any user-visible behavior change: a one-line entry in the PR description describing what the user will now see or do.

---

## 5. Agent operational rules

- **On ambiguity, stop and ask.** Do not guess at scope, naming, or behavior. Prefer one clarifying question over three wrong commits.
- **Never commit model weights.** Use `tools/fetch_models.cmake`. If a test needs a model, the test harness fetches it; it is never checked in.
- **Never commit secrets, API keys, or anything under `~/` that isn't this repo.**
- **Never modify `bird_photo_ai_project_handoff.md` to match code.** If reality diverges from the plan, flag it to the user and wait for a decision.
- **Do not introduce new dependencies** without adding them to `vcpkg.json` and updating `docs/LICENSING.md`. Net-new dependencies require user approval.
- **Do not exceed module responsibilities.** If a task in module X requires a change in module Y, open a separate task for Y rather than quietly editing across boundaries.
- **Prefer readability over cleverness.** See NFR-6. If you're reaching for SFINAE, CRTP, or heavy templates, stop and consider a concrete alternative first.
- **Third-party exceptions do not cross module boundaries.** Catch Exiv2/LibRaw/ONNX Runtime/Qt exceptions at the module's public API and translate to Wildframe error types (see NFR-7 exception policy).

---

## 6. What's explicitly out of scope for MVP

Do not build these unless the user has changed the handoff document:

- Species identification, eye/head detection, action/context tags.
- RAW formats other than **CR3**.
- Lightroom plugin, raw-ingest GUI integration.
- Windows/Linux support, mobile support.
- Custom model training (no Python in the delivered product).
- SQLite or any internal DB — XMP sidecars are the source of truth.
- Image editing, DAM features.

---

## 7. Working loop

When starting a task:

1. Read the task in `docs/BACKLOG.md`. Note its dependencies and acceptance criteria.
2. Verify dependencies are complete (their tasks are checked off). If not, stop.
3. Re-read the relevant FR/NFR from the handoff doc.
4. Create a branch per `CONTRIBUTING.md`.
5. Implement. Run format, tidy, and tests locally. Iterate until clean.
6. Open a PR citing FR/NFR and task ID.
7. Mark the backlog item complete **only after** the PR is merged.
