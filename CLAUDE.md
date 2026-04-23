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
| Runtime TOML config keys and defaults | `docs/CONFIG.md` |
| Test fixture catalog and provenance | `docs/FIXTURES.md` |
| Local dev environment setup (toolchain, vcpkg, presets) | `docs/DEV_SETUP.md` |
| Project overview for new contributors | `README.md` |
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
3. `cmake --build --preset tidy` produces **zero clang-tidy findings**. No new `NOLINT` suppressions without a justification comment that cites either (a) a confirmed false positive with evidence, or (b) an alternative fix that was *attempted and rejected*, with the reason. A NOLINT whose comment only describes the checker's complaint without documenting a tried-and-rejected real fix is a premature suppression; try the real fix first (catch the exception, narrow the type, fix the include hygiene) and only suppress when that path genuinely fails.
4. `format-check` passes.
5. Public headers have brief Doxygen comments on their public interfaces.
6. The task's PR description cites the FR-x and/or NFR-x it satisfies, from `bird_photo_ai_project_handoff.md`.
7. For any user-visible behavior change: a one-line entry in the PR description describing what the user will now see or do.

---

## 5. Agent operational rules

- **On ambiguity, stop and ask.** Do not guess at scope, naming, or behavior. Prefer one clarifying question over three wrong commits.
- **Never commit model weights.** Use `tools/fetch_models.cmake`. If a test needs a model, the test harness fetches it; it is never checked in.
- **Never commit secrets, API keys, or anything under `~/` that isn't this repo.**
- **The plan is the source of truth, but it is not infallible.** If, while working a task, you identify a materially better approach than what the plan prescribes — one that better serves the stated goals and NFRs — **stop before implementing** and propose the plan change to the user. Do not silently deviate, and do not edit the handoff doc unilaterally. Wait for approval, then proceed via the two-step process in `CONTRIBUTING.md` (update plan → implement).
- **Scope the judgment.** Plan-change proposals are about *how* to achieve what the plan already says to achieve. They are **not** license to expand scope. "I think we should also build X" is scope creep, not a plan improvement, and belongs as a separate backlog addition with justification — never smuggled in under "making the best product."
- **Never modify `bird_photo_ai_project_handoff.md` to match code reality.** If reality has drifted from the plan without explicit approval, that's a bug in the code, not the plan. Flag it and wait for a decision.
- **Do not introduce new dependencies** without adding them to `vcpkg.json` and updating `docs/LICENSING.md`. Net-new dependencies require user approval.
- **Do not exceed module responsibilities.** If a task in module X requires a change in module Y, open a separate task for Y rather than quietly editing across boundaries.
- **Prefer readability over cleverness.** See NFR-6. If you're reaching for SFINAE, CRTP, or heavy templates, stop and consider a concrete alternative first.
- **Verify the library's idiomatic usage before designing.** For any non-trivial interaction with a third-party dependency (logging, HTTP, ORM, serialization, parsing, image I/O), read the library's own FAQ, README, or reference examples for its recommended pattern *before* sketching a Wildframe API layer over it. Do this as part of §7's judgment gate, not as a reaction to review feedback. The prior draft of `wildframe_log` invented a `WILDFRAME_LOG_*` macro fan-out instead of spelling the spdlog-blessed `logger->info(...)` call — an hour of spdlog-docs reading would have preempted a full redesign.
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
4. **Judgment gate.** Before writing code, ask: does the plan's prescribed approach still look like the best way to satisfy the cited FR/NFR? If you see a materially better approach, stop and propose the plan change per §5. If the prescribed approach is fine, proceed.
5. Create a branch per `CONTRIBUTING.md`.
6. Implement. Run format, tidy, and tests locally. Iterate until clean.
7. Open a PR citing FR/NFR and task ID. **Flip the backlog checkbox (`[ ]` → `[x]`) for the task inside the same PR.** If the PR is abandoned or closed unmerged, the checkbox change dies with it — no separate bookkeeping PR is needed.

---

## 8. Project health reviews

At milestones (Sprint 0 complete, each module complete, phase transitions, or any time something feels off), run a structured project health review.

**How to trigger:**

- Inside a Claude Code session: `/review <milestone-name>` (defined in `.claude/commands/review.md`).
- Or paste the trigger prompt below into a **fresh conversation**.

**Where output lives:**

- `docs/reviews/<YYYY-MM-DD>-<milestone-slug>.md`, following `docs/REVIEW_TEMPLATE.md`.
- Committed to the repo for audit trail.

**Scope rules for reviewing agents:**

- **Read-only.** Never modify code, never stage, never commit. Only write the new review file.
- **Propose, don't re-plan.** The handoff doc is the source of truth; surface deviation, don't redesign.
- **Cite evidence.** Every NFR claim, every backlog change, every dep proposal must point to a file:line, benchmark number, or specific finding.
- **Default reject on new deps.** Per NFR-6, new dependencies must clearly reduce maintenance surface to be worth adopting.

**Trigger prompt (paste into a fresh conversation):**

> You are performing a project health review of Wildframe at the milestone: `<milestone-name>`.
> Read `bird_photo_ai_project_handoff.md`, `CLAUDE.md`, `docs/BACKLOG.md`, `docs/REVIEW_TEMPLATE.md`, and any prior files in `docs/reviews/`.
> Examine the current codebase read-only. Produce a filled-in review at `docs/reviews/<YYYY-MM-DD>-<milestone-slug>.md` following the template exactly.
> You may use web search to assess dependency candidates (§2 of the template).
> Do not modify any file other than the new review file. Do not stage or commit.
> When done, reply with the path to the review file and the top three findings (one sentence each, tagged `critical` / `concern` / `fyi`).
