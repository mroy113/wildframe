# Project Health Review Template

Review agents copy this file to `docs/reviews/<YYYY-MM-DD>-<milestone-slug>.md` and fill in every section. Leave sections empty if there's genuinely nothing to report — do not pad.

Reviews are **advisory and read-only**. Every proposed change goes through the normal PR flow per `CONTRIBUTING.md`. A reviewing agent does not modify code directly.

---

## Metadata

- **Milestone:** e.g., "Sprint 0 complete", "M3 wildframe_detect complete", "pre-Phase 2"
- **Date:** YYYY-MM-DD
- **Commit reviewed:** full SHA
- **Reviewer agent:** model + role (e.g., "Claude Sonnet 4.6, review-only")
- **Scope examined:** which modules, directories, or task IDs were actually inspected

---

## 1. Plan Fidelity

Does the code match `bird_photo_ai_project_handoff.md` and `docs/BACKLOG.md`?

### Matches plan
Brief list of what conforms. One line each.

- …

### Silent deviations from plan
Anything in the code that wasn't in the plan. For each: what it is, where it lives (file:line), whether it's benign or should be reverted/surfaced.

- …

### Planned but missing
Items from the plan/backlog that should be present but aren't.

- …

---

## 2. Dependency Reassessment

For each module examined, ask: is there a mature library that would replace meaningful in-tree code? Weigh each candidate against **NFR-6** — new dep = maintenance surface, build complexity, licensing overhead.

**Default recommendation is *reject*.** Only propose adoption when the candidate clearly reduces maintenance surface.

For each candidate, report:

- **Module:** which module
- **In-tree code it would replace:** file:line range, LoC, purpose
- **Candidate library:** name, license, vcpkg availability, last release date, activity level
- **Pros vs building in-house**
- **Cons vs building in-house**
- **Recommendation:** adopt / reject / investigate further

### Candidates identified
- …

### Modules reviewed, no candidate worth considering
- `wildframe_<module>`: one-line reason.

---

## 3. NFR Drift

For each NFR in `bird_photo_ai_project_handoff.md` §9, assess current state. Back every claim with specific evidence — file paths, benchmark numbers, clang-tidy counts. No vague assessments.

| NFR | State | Evidence |
|---|---|---|
| NFR-1 Performance | on track / at risk / violated | benchmark numbers if available |
| NFR-2 Accuracy | | |
| NFR-3 Extensibility | | |
| NFR-4 Interoperability | | |
| NFR-5 Auditability | | |
| NFR-6 Maintainability (**primary**) | | cite specific examples — both clean and degraded |
| NFR-7 Code Standards | | clang-format clean? STYLE.md current? exception policy followed? |
| NFR-8 Static Analysis | | total suppressions, trend, any new unexplained ones |

### clang-tidy suppression audit
- Total suppressions: N
- New since last review: M
- Suppressions that warrant re-examination: list with file:line

### Readability spot-checks
Pick two or three files at random, report whether a new embedded C++ dev could understand them without help. Call out specific patterns that violate NFR-6 (heavy templates, clever over clear, etc.).

---

## 4. Risk Register Update

For each risk in `bird_photo_ai_project_handoff.md` §15, assess whether it has moved from "possible" to "observed."

### Risks elevated since last review
- …

### New risks discovered
- …

### Risks no longer relevant
- …

---

## 5. Backlog Grooming

Proposed changes to `docs/BACKLOG.md`. Every change must cite the finding from §1–§4 that justifies it. Do not invent new product scope.

### Re-order
- "Move X before Y because …"

### Re-size
- "Task M3-03 was estimated S, observed M. Size adjustment warranted because …"

### Add
New tasks warranted by findings. Each entry: proposed ID, title, rationale, link to finding.

- …

### Remove or mark obsolete
Tasks that should be struck through. Include one-line reason; never delete.

- …

---

## 6. Open Ambiguities

Questions the build agents couldn't resolve and that remain unanswered. Surface each so the customer can make the call.

- …

---

## Summary

One or two paragraphs. Overall project health, the one or two most important actions to take before the next review, anything worth watching.
