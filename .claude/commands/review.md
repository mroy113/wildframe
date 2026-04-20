---
description: Run a structured project health review at a milestone
argument-hint: <milestone-name>
allowed-tools: Read, Grep, Glob, Bash, WebFetch, WebSearch, Write
---

Perform a project health review of Wildframe at the milestone: **$ARGUMENTS**

You are the **review agent**. Your role is read-only: propose, never edit code.

## Required reading (in order)

1. `bird_photo_ai_project_handoff.md` — source of truth for scope, architecture, NFRs.
2. `CLAUDE.md` — operational rules, §8 for review scope.
3. `docs/BACKLOG.md` — current task state.
4. `docs/REVIEW_TEMPLATE.md` — the structure your output must follow exactly.
5. Any existing files in `docs/reviews/` — for continuity.
6. The current codebase under `libs/`, `src/`, and `tools/`, scoped to what this milestone covered.

## Process

1. Run `git rev-parse HEAD` to capture the commit SHA. Run `date +%Y-%m-%d` for the date.
2. Pick a short slug for the milestone argument (lowercase, hyphenated).
3. Create the review file at `docs/reviews/<date>-<milestone-slug>.md`, copying the template structure.
4. Fill in every section. Leave a section empty if there is genuinely nothing to report — do not pad.
5. For **§2 Dependency Reassessment**: use web search and vcpkg search to identify candidate libraries. For each, cite name, license, vcpkg availability, last release date, activity level. Default recommendation is *reject* unless the candidate clearly reduces maintenance surface per NFR-6.
6. For **§3 NFR Drift**: back every claim with specific evidence — file paths with line numbers, benchmark results, clang-tidy suppression counts. No vague assessments.
7. For **§5 Backlog Grooming**: every proposed task change must cite the finding from §1–§4 that justifies it.

## Scope discipline (hard rules)

- **Do not modify any file** except the new review file under `docs/reviews/`.
- **Do not stage, commit, push, or create branches.** The user lands the review file via the normal PR flow.
- **Do not invent new product scope.** Backlog additions must trace back to findings in §1–§4.
- **Do not re-plan.** The handoff document is the source of truth; your job is to surface deviation, not redesign.
- **Do not run builds or tests** that modify disk state outside `docs/reviews/`. Reading `build/` output is fine; producing new build artifacts is not.

## Reply format

When finished, post a reply containing exactly:

1. **Path to the review file you created.**
2. **Top three findings**, one sentence each, tagged with severity:
   - `critical` — blocks the current milestone or violates a primary NFR
   - `concern` — worth addressing soon; no immediate blocker
   - `fyi` — observation worth logging
3. **Anything you would have investigated further** with more time or tools — so the customer knows where the review is thinnest.

Do not summarize the full review in the reply; the file is the artifact.
