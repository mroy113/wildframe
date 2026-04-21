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
  type (`box.area()`, not `box.GetArea()`), matching Google's
  "short, trivial accessor" carve-out. Non-trivial accessors and all
  setters use the verb prefix (`LoadSession()`, `SetThreshold()`).
- `snake_case_` trailing-underscore applies to `private:` and
  `protected:` members of classes. Plain aggregates (`struct` used as a
  passive data carrier — e.g. `ImageJob`, `DetectionResult`) have no
  invariants to protect and use bare `snake_case`.

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

**TODO (S0-17):** Level semantics, default level per build type, module
tag conventions, and file rotation policy.
