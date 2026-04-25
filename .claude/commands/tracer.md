---
description: Work the next backlog task as a tracer bullet, PR it, watch CI
---

Work the next task or tracer-bullet slice from `docs/BACKLOG.md`.

## Reading order

`CLAUDE.md` is already loaded — do not re-read it.

1. `CONTRIBUTING.md`, `docs/STYLE.md`, `docs/ARCHITECTURE.md` — full read.
2. `docs/BACKLOG.md` — full read once, to pick a meaningful end-to-end slice. Then narrow to the chosen task block(s) plus listed deps.
3. `bird_photo_ai_project_handoff.md` — grep for the cited FR-x / NFR-x IDs; read only those sections. Do not propose edits to this file; it is the source of truth per CLAUDE.md §5.

## Before coding

- Verify task dependencies are checked off. If not, stop, investigate, and then ask about it.
- Stop and ask whenever scope, naming, or behavior is ambiguous — up front or mid-task. Prefer one clarifying question over three wrong commits.

## Implement

- Create a branch per `CONTRIBUTING.md`.
- Iterate locally until `format-check`, `tidy`, and `ctest` are all green per `CLAUDE.md` §4 Definition of Done.
- Open a PR citing FR/NFR and task ID. Flip the backlog checkbox (`[ ]` → `[x]`) in the same PR.

## After the PR is open

Poll CI and surface status. Cadence: a single ScheduleWakeup at ~240s (inside the 5-min prompt cache window) usually catches a 3–5 min CI run in one extra turn — preferable to looping at 20s, which burns tool-call turns. Drop to 60–120s only when you expect status to flip soon (waiting on a single retry, etc.). On failure: diagnose, propose a fix, wait for my go-ahead before pushing further commits. Do not silently retry.

**Stop after the PR is green.** Do not pick up the next backlog task without an explicit go-ahead. One `/tracer` invocation = one task. After surfacing the green PR, wait for my next instruction (could be "done", "next", merge feedback, a different task, or something else entirely).

## When I say "done" (or similar)

Review the full conversation and propose updates to: your memory, `CONTRIBUTING.md`, `docs/STYLE.md`, `docs/ARCHITECTURE.md`, `docs/BACKLOG.md`, `CLAUDE.md`, or any other doc that would make the next run smoother and cheaper in tokens. Keep suggestions general enough to stay useful across tasks. Show proposed diffs before writing. Keep an eye out for repeated permission requests for build, static analysis, and test commands - these should be resolved as well.
