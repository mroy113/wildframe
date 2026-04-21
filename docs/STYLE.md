# Wildframe C++ Style

This file is the contract for mechanical formatting and, from task S0-07
onward, the broader code-style policy (naming, header ordering, exception
policy, `noexcept`, smart pointers, etc.).

At this stage only the formatter baseline is pinned. The remaining
sections are placeholders until S0-07 lands.

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

## 2. Naming, header ordering, smart pointers, `auto`, RAII, `noexcept`

**TODO (S0-07):** Resolve Core Guidelines vs Google conflicts per
handoff §NFR-7. Every resolution cites why.

---

## 3. Exception policy

**TODO (S0-07):** Mirror handoff §NFR-7 — exceptions allowed inside
module internals but must not cross module public API boundaries.
Third-party exceptions (Exiv2 / LibRaw / ONNX Runtime / Qt) are caught
at the boundary and translated to Wildframe error types.

---

## 4. Logging

**TODO (S0-17):** Level semantics, default level per build type, module
tag conventions, and file rotation policy.
