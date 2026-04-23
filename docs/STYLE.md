# Wildframe C++ Style

This file is the contract for mechanical formatting, naming, header
ordering, smart-pointer conventions, `auto`, RAII, `noexcept`, and the
exception policy. It is the resolution record for points where the
C++ Core Guidelines and the Google C++ Style Guide disagree, per
handoff §NFR-7.

**Baselines (both authoritative, consulted in this order):**

1. **C++ Core Guidelines** — https://isocpp.github.io/CppCoreGuidelines/
2. **Google C++ Style Guide** — https://google.github.io/styleguide/cppguide.html

Where they conflict, this document records which one wins on the
disputed point and why. Anything not called out here follows both
(they agree on the vast majority of ground).

All resolutions are driven by one principle from handoff §NFR-6:
**maintainability wins**. Where a rule trades readability for
cleverness, brevity, or micro-performance, readability wins unless a
benchmark proves the trade is required.

---

## 1. Mechanical formatting (S0-05)

- **Baseline:** Google C++ style, as enforced by `clang-format` via the
  repo-root `.clang-format` file.
- **Language standard:** C++20 (matches the `CMAKE_CXX_STANDARD` set in
  the top-level `CMakeLists.txt`).
- **Scope:** all `*.cpp` / `*.hpp` / `*.h` / `*.cc` files under `libs/`,
  `src/`, and `tests/`. Third-party code vendored under `external/` is
  out of scope and must not be reformatted.

### Deviations from Google style

None at this time. Keeping the formatter baseline vanilla minimizes
argument surface and matches NFR-6 (maintainability — the average
embedded C++ developer already knows Google style).

Any future deviation must be recorded here **before** it is added to
`.clang-format`, with:

1. The concrete `clang-format` option being changed.
2. The reason, tied to an FR or NFR.
3. An alternative that was considered and rejected.

### Running the formatter

```bash
# Configure first (any preset works — presets share the top-level targets).
cmake --preset debug

# Auto-format every in-scope file in place.
cmake --build --preset debug --target format

# Verify every in-scope file is already formatted; non-zero exit on drift.
cmake --build --preset debug --target format-check
```

`format-check` is the gate CI runs per NFR-7; `format` is a local-only
convenience for applying the fix.

---

## 2. Code conventions

Each subsection below records a resolution: the rule Wildframe follows,
which guide it comes from, and the one-line reason the other was
rejected. If a topic has no Core Guidelines / Google conflict, the
resolution is simply "both agree" and no justification is needed.

### 2.1 Naming

**Google wins.** Consistency with the wider C++ ecosystem that embedded
developers already read (LLVM, Chrome, TensorFlow, Abseil) outranks any
marginal benefit from an alternative.

| Entity | Convention | Example |
|---|---|---|
| Types, classes, structs, enums, type aliases | `CamelCase` | `ImageJob`, `DetectionResult` |
| Functions (free and member) | `CamelCase` | `EnumerateDirectory()`, `WriteSidecar()` |
| Variables (local, parameters, file-scope) | `snake_case` | `bird_count`, `raw_path` |
| Non-public class data members | `snake_case_` (trailing underscore) | `confidence_`, `session_` |
| Public struct data members (plain aggregates only) | `snake_case` (no trailing underscore) | `bird_present`, `bird_boxes` |
| Compile-time constants, `constexpr`, `const` file-scope | `kCamelCase` | `kDefaultConfidenceThreshold` |
| Namespaces | `lower_snake_case` | `wildframe::ingest` |
| Enum values (scoped enums only — unscoped enums are banned by Google) | `kCamelCase` | `Format::kCr3` |
| Macros (reserve for genuine preprocessor needs) | `WILDFRAME_SCREAMING_SNAKE` | `WILDFRAME_LOG_MODULE` |
| File names | `lower_snake_case.{hpp,cpp}` | `image_job.hpp` |

**Notes.**

- Getters drop the `Get` prefix when they are pure accessors on a value
  type (`box.Area()`, not `box.GetArea()`), matching Google's
  "short, trivial accessor" carve-out. Non-trivial accessors and all
  setters use the verb prefix (`LoadSession()`, `SetThreshold()`).
- `snake_case_` trailing-underscore applies to `private:` and
  `protected:` members of classes. Plain aggregates (`struct` used as a
  passive data carrier — e.g. `ImageJob`, `DetectionResult`) have no
  invariants to protect and use bare `snake_case`.
- **Single-letter names are reserved for universal conventions** —
  loop indices and iterators (`i`, `j`, `it`), coordinate components
  (`x`, `y`, `z`), template type parameters (`T`, `U`), and the
  exception caught in `catch (const std::exception& e)`. Everything
  else spells out its role (`input`, `count`, `remainder`,
  `home_path`), because the semantic meaning that a single letter
  discards is exactly what the reader needs to navigate the code.
  If a descriptive name feels too long, find a better short word —
  do not shrink to one letter.

### 2.2 Header ordering

**Google wins.** Google's prescribed include order is mechanical,
enforceable by `clang-format` (`IncludeCategories`), and catches
missing-include bugs by forcing the owning header to compile first.

Order in every `.cpp` file, with a blank line between groups:

1. The matching header for this translation unit (e.g. `foo.cpp`
   starts with `#include "wildframe/foo/foo.hpp"`).
2. C system headers (`<unistd.h>`, `<sys/stat.h>`).
3. C++ standard library headers (`<filesystem>`, `<string>`).
4. Third-party library headers, each group separated:
   Qt, LibRaw, Exiv2, OpenCV, ONNX Runtime, spdlog, nlohmann/json, GoogleTest.
5. Wildframe project headers (`#include "wildframe/<module>/<name>.hpp"`).

Within each group, sort alphabetically.

Headers (`.hpp`) follow the same order but omit group 1. Every header
is self-contained: it `#include`s everything it needs and uses
`#pragma once`.

### 2.3 Smart pointers and ownership

**Core Guidelines wins (R.20, R.21, R.22, R.23, F.7).** Google's older
discouragement of smart pointers predates widely-available C++17 and
has been overtaken by common practice; Core Guidelines' ownership model
is the clearer contract.

- **Default to value semantics.** If a type can be held by value, hold
  it by value.
- **Owning pointer → `std::unique_ptr<T>`.** Transfer with `std::move`.
- **`std::shared_ptr<T>` only when ownership is genuinely shared** —
  i.e. multiple independent lifetimes legitimately keep the object
  alive. "I didn't want to think about ownership" is not a reason.
  Prefer redesign.
- **Non-owning reference → `T&`, `const T&`, or `T*`** (nullable, non-
  owning). Never pass `unique_ptr<T>&` when `T*` will do (F.7).
- **Construct with `std::make_unique` / `std::make_shared`.** Never
  bare `new`. Never bare `delete` — RAII wraps every resource.
- **View types for ranges of bytes or characters:** `std::span<T>`,
  `std::string_view`. Pass these by value, not by reference.

### 2.4 `auto`

**Google wins (narrowly).** Both guides accept `auto` for type
deduction; Google's more conservative framing ("use it where it aids
the reader, not where it hides the type") is a better fit for a
codebase whose primary NFR is maintainability.

**Use `auto` when the concrete type is noisy or obvious:**

```cpp
auto it = images.find(path);                     // iterator — type is obvious.
auto session = std::make_unique<OrtSession>(...); // RHS states the type.
for (const auto& job : jobs) { ... }             // range-for convention.
```

**Prefer the explicit type when the numeric width, signedness, or
semantic meaning matters:**

```cpp
std::size_t bird_count = boxes.size();  // not auto — width matters.
float confidence = 0.0F;                // not auto — literal is double.
std::int32_t focal_length = ...;        // not auto — API contract.
```

Never `auto` a return value whose type the reader cannot infer from
the call site in two seconds.

### 2.5 RAII

**Both agree.** Codified here as a hard rule:

- Every resource (file handle, mutex lock, LibRaw handle, ONNX
  session, Qt widget ownership) is owned by a type whose destructor
  releases it. No bare `new`/`delete` in application code.
- **Rule of Zero** for value-type aggregates: rely on compiler-
  generated special members. Do not write them.
- **Rule of Five** for resource-owning types: if a class manages a
  resource directly, it declares all five special members (or
  `= default`s / `= delete`s them explicitly). Partial declarations
  (e.g. custom destructor without custom copy/move) are a clang-tidy
  error (`cppcoreguidelines-special-member-functions`).
- Third-party C handles (LibRaw, Exiv2 C API) are wrapped in a thin
  RAII class at the module boundary. The rest of the module never
  touches the C handle.

### 2.6 `noexcept`

**Core Guidelines wins (F.6).** Google is cautious about `noexcept`
because pre-C++11 codebases mixed it with legacy exception
specifications; that concern is moot in a greenfield C++20 project.

Mark `noexcept` when the function **genuinely cannot throw**. Do not
blanket-mark functions that call into code paths that may throw, and
do not mark `noexcept(false)` — the default is already "may throw".

Required `noexcept` sites:

- **Move constructor and move assignment** of any class that owns a
  resource. Containers depend on this for the strong exception
  guarantee (`performance-noexcept-move-constructor` enforces it).
- **Destructors.** Implicit, but be explicit on polymorphic bases.
- **`swap` (free function and member).** Required by the standard
  library's swap-idiom expectations.
- **Trivial value-semantic getters** on POD-ish types:
  `constexpr int Width() const noexcept { return w_; }`.

Do **not** mark `noexcept`:

- Functions that call third-party C++ APIs (LibRaw, Exiv2, ONNX
  Runtime, Qt). Those can throw, and catching-at-boundary (§3) means
  the function can throw until the boundary.
- Functions that perform allocation that is not guaranteed to succeed.

### 2.7 Readability cap (NFR-6 reminder)

Independent of the resolutions above, NFR-6 forbids reaching for
advanced C++ features without a concrete reason that ties to an FR/
NFR. The following are presumed-no until the PR description argues
otherwise:

- SFINAE, `std::enable_if` plumbing.
- CRTP.
- Expression templates.
- Custom allocators.
- Heavy template metaprogramming (more than one `template<>`
  specialization per type, or recursive instantiation).
- Operator overloading beyond arithmetic types and iostream.

When a straightforward concrete implementation works, use it.

### 2.8 Macros

Macros are the **last resort**, not a convenience. Use one only when
the feature you need cannot be expressed as a function, a `constexpr`
value, an `inline` variable, or a template — the prototypical case
being preprocessor-level conditional compilation that must see or
discard token sequences before they are parsed (e.g. `spdlog`'s
`SPDLOG_ACTIVE_LEVEL` compile-time level strip in §4.2, which
requires the argument list to literally disappear from the
translation unit). If a non-macro alternative exists at all, prefer
it, even at some verbosity cost at the call site.

When a macro is justified:

- Scope the `NOLINT(cppcoreguidelines-macro-usage)` suppression to
  exactly the macro (or `NOLINTBEGIN`/`NOLINTEND` block covering
  just the macro definitions), never file-wide.
- Leave a comment at the definition stating which non-macro
  alternative was considered and why it is insufficient.

### 2.9 Wrapping dependency types

Do not wrap a backend library's public types (enums, status codes,
error types, option structs) in a parallel Wildframe type unless the
wrapper does **at least one** of:

- (a) Narrow the surface area exposed to callers — e.g. hiding a
  large enum behind a curated subset we actually use.
- (b) Decouple callers from a dep we genuinely intend to swap — not
  a theoretical "what if we change libraries someday" motivation,
  but one tied to a concrete forecast in the handoff doc or backlog.
- (c) Add invariants the backend type lacks — e.g. a constructor
  that rejects values the backend would accept.

A wrapper that only renames symbols for namespace aesthetics, or
"keeps our API clean" without narrowing, decoupling, or adding
invariants, is NFR-6 debt. Type aliases (`using level = spdlog::level;`)
are the honest way to shorten a qualified name without the
maintenance cost of a real wrapping layer.

### 2.10 `NOLINT` is a last resort

A `NOLINT` / `NOLINTNEXTLINE` / `NOLINTBEGIN` suppression is a
statement that the checker is wrong about this specific site. It
requires evidence, not just a rationale.

Before suppressing any clang-tidy finding, try the real fix:

- Catch the exception the checker worries about.
- Narrow the type so the unchecked operation becomes safe.
- Restructure the call site (e.g. `s.starts_with('~')` instead of
  `s[0] == '~'`) so the check no longer applies.
- Add an invariant (`ASSERT`/`if` guard, dependency injection) that
  makes the operation provably safe.

Only when the real fix is genuinely non-viable may a `NOLINT` ship,
and the comment at the suppression site must say **what real fix was
attempted and why it was rejected**. A comment that merely restates
the checker's complaint ("getenv is flagged mt-unsafe, but it's
fine") is a premature suppression — the real fix was not tried.

Scoping rules:

- Always prefer `NOLINTNEXTLINE` (or a tight `NOLINTBEGIN`/`END` pair)
  over file-wide `// NOLINT(check)` banners. The suppression must be
  scoped to the exact line(s) the fix-attempt could not cover.
- Name the specific check (`NOLINTNEXTLINE(concurrency-mt-unsafe)`).
  Bare `NOLINT` / `NOLINT(*)` is banned — it silences checks the
  suppression author never considered.

### 2.11 Do not ship speculative interface surface

A task's public interface covers exactly the callers that exist
inside the task's own scope. Exported functions, headers, config
keys, and error types that are "prepared" for a caller that will
land in a future task are NFR-6 debt: they must be understood,
formatted, tidied, and kept in sync forever, for zero present
value, and by the time the future caller arrives its actual shape
almost always diverges from the prepared one.

Concretely:

- If the only reference to a new public function lives inside the
  same module's own tests, the function is speculative — delete it
  and let the task that introduces the real caller introduce the
  seam in the shape it actually needs.
- If a config key is defined but no code reads it in this task,
  defer the key. `docs/CONFIG.md` can describe the intended default,
  but the parser wiring lands with the first consumer.
- Internal helpers used by the same `.cpp` file are fine — they are
  not interface surface. This rule is about things that cross a
  module boundary or a public header.

Exception: load-bearing invariants that the task's own code depends
on, even if only at one call site today, are not speculative. The
line between "one caller today" (fine) and "zero callers today"
(debt) is the test.

### 2.12 Environment and global state reads

Process-wide state — environment variables, current working
directory, wall-clock time, locale, per-process singletons —
enters the program at exactly one known boundary per kind of state,
and is threaded downward as a parameter from there. Business logic
never calls `std::getenv`, `std::filesystem::current_path`, or
`std::chrono::system_clock::now` from the middle of a function.

This is a testability rule, not a style affectation: functions that
take resolved values as parameters are unit-testable with injected
data and need no `setenv` / `chdir` / clock-mocking in their test
harness. A test that mutates the process environment is a smell —
it is global state shared with every other test in the same
executable and with `ctest` itself, and it means the function under
test took on a dependency that should have been a parameter.

Concretely:

- The thin seam that reads `$HOME`, `$XDG_CONFIG_HOME`, etc. lives
  in the startup wiring (main, or the orchestrator's one-shot
  config resolver). Every other function takes a resolved
  `std::filesystem::path` or equivalent.
- If you find yourself wanting to mock `std::getenv` in a test,
  stop and refactor the code instead: the function should have
  taken the value as a parameter.
- The same applies to wall-clock time and working directory.
  Manifest writers, log-file rotators, and anything else that
  cares about "now" or "where" takes an injected clock or path and
  lets the integration point supply the process-level value.

### 2.13 GoogleTest ↔ clang-tidy interactions

A few clang-tidy checks do not model gtest-specific idioms. Rather
than contort every test file to satisfy checks that were never aimed
at test code, we disable the worst offenders on test executables
only. Production code under `libs/<module>/src/` and
`libs/<module>/include/` still runs the full check set.

**Mechanism.** A single cmake helper holds the tests-only disable
list: `wildframe_apply_tests_tidy_config(<target>)` in
[cmake/ClangTooling.cmake](../cmake/ClangTooling.cmake). Every test
`CMakeLists.txt` calls it once, right after `add_executable`. The
helper overrides the target's `CXX_CLANG_TIDY` to add
`--checks=-foo,-bar,...` on top of the repo-root `.clang-tidy`.
There is exactly one place to update the disable list; no per-test
config files.

**Disabled on test targets:**

- `cppcoreguidelines-non-private-member-variables-in-classes` and
  its alias `misc-non-private-member-variables-in-classes`.
  Idiomatic gtest fixtures put shared state in `protected:` so
  derived `TEST_F` bodies can reach it; the check treats every
  non-`private` member as encapsulation debt. Forcing `private:` +
  `protected:` accessor pairs on every fixture is ceremony that no
  reader benefits from.
- `readability-function-cognitive-complexity`. Each
  `EXPECT_*`/`ASSERT_*` expands to a nested switch/if that adds to
  the score, so a straightforward "check every element" loop
  exceeds the default threshold of 25 with four or more assertions.
  Rather than extract helpers purely to appease the expansion, let
  test bodies stay linear.

**Still enforced — with a documented workaround:**

- `bugprone-unchecked-optional-access` does not follow `ASSERT_TRUE`.
  The checker's dataflow analysis cannot see through gtest's
  aborts-the-caller macro contract, so an
  `ASSERT_TRUE(opt.has_value())` followed by `*opt` still trips.
  This one is kept because the failure mode (dereferencing an empty
  optional) is a real bug class outside gtest too. Pair the
  assertion with a plain `if (opt.has_value())` around the
  dereference:
  ```cpp
  ASSERT_TRUE(job.size_bytes.has_value());
  if (job.size_bytes.has_value()) {
    EXPECT_GT(*job.size_bytes, 0U);
  }
  ```

---

## 3. Exception policy

Mirrors handoff §NFR-7. **Core Guidelines wins** over the historical
Google ban; Wildframe is a greenfield C++20 application, the ban's
original ABI-compatibility rationale does not apply, and the Core
Guidelines E-series gives a clearer contract than error-code
propagation through seven layers.

### 3.1 Where exceptions live

- **Allowed and expected inside module internals.** Implementation code
  inside `libs/<module>/src/` may throw and catch freely.
- **Not allowed across module public API boundaries.** A function
  declared in a module's public header (`libs/<module>/include/...`)
  may only throw Wildframe-defined error types, and only the types
  documented in that header.
- **Third-party exception types never cross the boundary.** LibRaw,
  Exiv2, OpenCV (`cv::Exception`), ONNX Runtime (`Ort::Exception`), and
  Qt exceptions are caught at the module's public API and translated
  to the module's own error type. Catch sites live in the thin
  boundary layer, not scattered throughout the module.

### 3.2 Wildframe error types

Each module defines its own error hierarchy, rooted in
`wildframe::<module>::<Module>Error`, derived from
`std::runtime_error`.

```cpp
namespace wildframe::raw {
/// Raised by wildframe_raw public APIs for any decode failure.
/// Translates LibRaw's numeric error codes and any C++ exception
/// LibRaw's C++ wrapper raises.
class RawDecodeError : public std::runtime_error { ... };
}  // namespace wildframe::raw
```

Every public header documents, under the function's Doxygen `\throws`
tag, which Wildframe error types it may throw. A function that throws
something not listed is a bug.

### 3.3 Guarantees

- **Basic exception safety** is required on every public API: on
  throw, no resources leak and the program remains in a valid (if
  possibly empty) state.
- **Strong exception safety** (commit-or-rollback) is required where
  practical — notably the XMP sidecar writer (M5-08), which uses the
  temp-file-plus-rename idiom to give atomic semantics against
  partial-write crashes.
- **`noexcept`-safe layers** — the orchestrator's per-job error
  isolation (M6-04) assumes a single job throwing will not destabilize
  the worker thread. It catches Wildframe error types, logs the job
  as failed in the batch manifest, and proceeds to the next job.

### 3.4 Usage rules

- **Exceptions are for exceptional conditions only.** A malformed CR3
  file in a batch of 500 is *expected* — it is not exceptional, it is
  handled by returning a validation result from the ingest stage. A
  corrupt ONNX model file is *exceptional* — throw.
- **Never use exceptions for control flow.**
- **`throw;` to rethrow, not `throw e;`** (`bugprone-use-after-move`
  and `cert-err09-cpp` enforce slicing prevention).
- **Catch by `const` reference.** Never by value.
- **No empty `catch` blocks.** If a failure is truly ignorable, say so
  with a one-line comment explaining why the state is safe to ignore.

### 3.5 Assertions vs exceptions

- `assert` / `static_assert` for **programming errors that should be
  impossible** — precondition violations in purely internal code,
  invariant checks that would indicate a bug. Debug-only; not a
  security boundary.
- Exceptions for **runtime conditions the caller cannot prevent** —
  missing files, malformed data, decoder failures, OOM.
- **Never use `assert` to validate external input.** Input from disk,
  the GUI, or a config file is always runtime.

---

## 4. Logging

All application logging goes through **spdlog** (handoff §5, §NFR-5).
S0-14 initializes one process-global pair of sinks — stdout and a
daily-rotating file — both thread-safe. Agents add log lines by
picking a level from §4.1 and using the module-tagged logger handle
in §4.5.

### 4.1 Level semantics

Wildframe uses all six spdlog levels. Pick the level that describes
how much a reader would want the line later during an incident
investigation or code review.

| Level | Intent | Example |
|---|---|---|
| `trace` | Inner-loop diagnostics. Compiled out of release binaries via `SPDLOG_ACTIVE_LEVEL` (§4.2). Use when you would `printf` a tensor shape while debugging but would not want the line surviving a batch of 500 images. | Per-stage byte counts during preview decode; pre-NMS box counts in `wildframe_detect`. |
| `debug` | Per-image or per-stage facts useful to an engineer reconstructing what happened, too noisy for users. Default on in `debug` build, off in `release`. | "detect stage produced 3 boxes, primary confidence 0.81"; "read_exif: 23 tags, focal_length=400mm". |
| `info` | Once-per-batch or once-per-run milestones an operator expects to see in the log. Default on in both builds. | "batch started: 147 CR3 files, reanalysis_policy=skip"; "pipeline_version=0.1.0 detector=yolov11 v11.0.0 ep=coreml". |
| `warn` | Something is off but the pipeline continued. A reader should be able to scan `warn` lines to find every skipped file without reading lower levels. | "skipped malformed CR3: /path/IMG_0001.CR3 — magic bytes mismatch"; "coreml EP unavailable, falling back to CPU". |
| `error` | A single job failed and the orchestrator wrote a failure row to the batch manifest (M6-04). Exactly **one** `error` line per failed job, including the job path. | "job failed: /path/IMG_0042.CR3 — RawDecodeError: LibRaw rc=-2 (Unsupported file format)". |
| `critical` | The batch is aborting or the process is going down. Reserved for M6-04's catastrophic conditions (OOM, corrupt ONNX model, missing startup asset). At most a handful per run. | "ONNX model load failed: weights/yolov11.onnx — session init threw"; "aborting batch". |

**Do not use `error` for expected per-image skips** (malformed CR3,
unreadable path). Those are `warn` — they are the expected outcome of
§3.4 "exceptions are for exceptional conditions only" translated to
the logging layer. `error` means an orchestrator manifest row was
written; `warn` means the image was dropped earlier without producing
a row.

### 4.2 Default thresholds

| Build | stdout sink | file sink |
|---|---|---|
| `debug` preset | `debug` | `debug` |
| `release` preset | `info` | `info` |
| `asan` preset | `debug` | `debug` |
| Tests under `ctest` | `warn` | disabled (tests do not rotate the user's log file) |

Both sinks share a level in a given build so a reader never has to
correlate across sinks to reconstruct what happened. Overridable per
run via the TOML config key `log_level` (S0-18) for field debugging.

`trace` requires a rebuild: `SPDLOG_ACTIVE_LEVEL` is set at compile
time to `SPDLOG_LEVEL_DEBUG` for `release` and `SPDLOG_LEVEL_TRACE`
for `debug`/`asan`, so `trace` lines are eliminated from release
binaries entirely. This is spdlog's recommended way to keep zero-cost
diagnostics in hot paths.

### 4.3 Log destination and rotation

Defaults locked by handoff §25 and S0-19:

- **Path:** `~/Library/Logs/Wildframe/wildframe.log`.
- **Rotation:** daily, at local midnight.
- **Retention:** 14 rotated files — older rotations are deleted on
  the next rotation.
- **Override:** TOML config key `log_path` (S0-18, S0-19).

Implementation note for S0-14: use
`spdlog::sinks::daily_file_sink_mt` with `max_files=14`. Do not write
to stderr — Wildframe is GUI-first and stderr is not user-visible.
The rotating file sink is the audit trail per NFR-5; the stdout sink
exists for developers running from a terminal, not for end users.

### 4.4 Log line format

```
<YYYY-MM-DD HH:MM:SS.mmm> [<level>] [<module>] <message>
```

Concrete `spdlog::pattern`:

```
%Y-%m-%d %H:%M:%S.%e [%^%l%$] [%n] %v
```

- `%n` is the spdlog logger name, which carries the module tag (§4.5).
- `%^%l%$` colorizes the level on the stdout sink; spdlog strips the
  color markers on non-tty sinks, so the file sink uses the same
  pattern verbatim.
- Timestamps are local time to match the user's mental model of when
  they ran a batch. The batch manifest (M6-05) records its own
  ISO-8601 timestamps for machine consumption.

Thread id (`%t`) is intentionally omitted: the MVP threading model
(ARCHITECTURE.md §5) has one worker plus the Qt main thread, so the
module tag is already unambiguous. Add `%t` only if a Phase 2
multi-consumer queue lands.

### 4.5 Module tag conventions

Every log line identifies the emitting module. The tag is the module
name from handoff §10 without the `wildframe_` prefix:

| Module | Tag |
|---|---|
| `wildframe_ingest` | `ingest` |
| `wildframe_raw` | `raw` |
| `wildframe_detect` | `detect` |
| `wildframe_focus` | `focus` |
| `wildframe_metadata` | `metadata` |
| `wildframe_orchestrator` | `orchestrator` |
| `wildframe` GUI | `gui` |

Build-system helpers (`tools/fetch_models.cmake`,
`tools/fetch_fixtures.cmake`) run at CMake configure time and are out
of scope — they use `message(STATUS …)` / `message(FATAL_ERROR …)`,
not spdlog.

**How a module picks up its tag.** S0-14 exposes one pre-registered
`wildframe::log::Logger` handle per module tag at namespace scope.
Callers use the handle's member functions for `info` / `warn` /
`error` / `critical` — the function API is spdlog's idiomatic style
(see the [spdlog FAQ][spdlog-faq]) and does not require per-TU
`#define` plumbing.

```cpp
// libs/detect/src/detect.cpp
#include "wildframe/log/log.hpp"

namespace log = wildframe::log;

log::detect.info("detect ready: ep={}", ep_name);
log::detect.warn("skipped frame, confidence {} below threshold",
                 score);
```

Only `trace` and `debug` go through preprocessor macros —
`WF_TRACE(handle, ...)` and `WF_DEBUG(handle, ...)` — because
compile-time stripping via `SPDLOG_ACTIVE_LEVEL` (§4.2) requires
macros. A function call would always compile, keeping trace-level
format cost in release binaries; the macro form lets the
preprocessor delete the call entirely.

```cpp
WF_TRACE(log::detect, "pre-NMS box count={}", n);
```

Agents use `wildframe::log::<tag>.*()` (and `WF_TRACE` / `WF_DEBUG`
for `trace` / `debug`) — not `spdlog::` APIs directly — so the
backing sink(s) can change later without touching callers.

Do **not** embed the tag into the message string itself
(`"[detect] …"`). The pattern already carries it in `[%n]`;
duplicating doubles it up in every rotated file.

[spdlog-faq]: https://github.com/gabime/spdlog/wiki/FAQ

### 4.6 What not to log

- **Pixel buffers, raw bytes, large JSON blobs.** Log sizes and
  hashes instead. A 24-megapixel preview base64-dumped once per image
  would flood a rotated file within a single batch.
- **XMP sidecar content.** The sidecar is the audit record (handoff
  §13) — logging it is redundant and inflates log size.
- **Paths inside tight loops.** Logging the job path once per stage
  at `debug` is fine; per-pixel-row logging is not.
- **Third-party stack traces at `info` or above.** Catch at the
  module boundary (§3.1), translate to a Wildframe error type, and
  log the translated error with the job path at `warn` / `error`.
  Stack traces belong at `trace` and only when actively debugging.

### 4.7 Interaction with the exception policy

§3.4 forbids exceptions for control flow; §4 forbids using log levels
as control flow either. Two concrete rules:

- **Inner log + translated error is enough.** When a module's
  implementation is about to throw, log the condition once at the
  right level (`warn` if per-image-expected; `error` or `critical`
  if pipeline-level), then throw. The boundary translator catches
  the third-party exception and rethrows the Wildframe error type
  (§3.1) — it does **not** log again.
- **The orchestrator logs exactly once per job failure at `error`**
  when it writes the manifest row (M6-04). Modules below it do not
  also emit `error` for the same failure.
