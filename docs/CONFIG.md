# Wildframe Runtime Configuration

This document is the **contract** for Wildframe's runtime configuration
file: every user-tunable setting, its type, its range, its default, and
the FR/NFR it serves. It is the companion to handoff §FR-11 and the
specification S0-18 delivers. Implementation (path-resolution helpers,
parsing, per-module config struct wiring) lands in S0-19 and downstream
module tasks; this file ships ahead of them so the key names are frozen
before any code reads them.

Where this file and the handoff doc disagree, the **handoff doc wins**
— flag the drift per `CLAUDE.md` §5 rather than editing either file to
paper over it.

**Scope.** MVP only. Phase 2+ modules (`wildframe_species`,
`wildframe_feedback`, `wildframe_lightroom`) will append sections here
when they land.

---

## 1. File format and location

### 1.1 Format

**TOML** (handoff §5 dependency table, §22 decision record). Parsed at
application startup with `tomlplusplus` (pinned in [vcpkg.json](../vcpkg.json)).

### 1.2 Resolution order

Wildframe resolves the config path in the following order. The first
hit wins; later candidates are not merged.

1. **CLI:** `wildframe --config <path>` — any path the user names, used
   as-is. A missing file here is a startup error: the user asked for a
   specific file and we should not silently fall through.
2. **User config:** `$XDG_CONFIG_HOME/wildframe/config.toml`. On macOS,
   `XDG_CONFIG_HOME` is typically unset; Wildframe falls back to
   `~/Library/Application Support/Wildframe/config.toml` in that case,
   matching the Apple-recommended location for per-app user data.
3. **Built-in default:** the values shipped in [`data/config.default.toml`](../data/config.default.toml).
   Used verbatim when no user file is present; also used as the
   defaulting source for individual missing keys in a partial user
   file (§1.4).

**No hot-reload.** Config is read once at startup. Changing the file
after Wildframe launches has no effect until the next launch (handoff
§FR-11).

### 1.3 Path expansion

String values naming a filesystem location expand a leading `~` to the
current user's home directory (`$HOME`). No other shell expansions are
performed: `$VAR`, `${VAR}`, globs, and backticks are taken literally.
This keeps the config readable without inheriting a shell's expansion
surface.

### 1.4 Partial user files

A user config file does not need to name every key. Missing keys fall
back to the built-in defaults (§1.2 step 3). This lets a user override
one setting (`log_level = "debug"`) without copying the entire default
file.

### 1.5 Unknown keys

Unknown top-level keys and unknown keys within a known table are a
**startup error**. A typo must fail loudly, not silently fall back to
the default — a config that reads `detction_confidence_threshold = 0.5`
must not run with the wrong threshold while the user thinks they
changed it. The error message names the offending key and points at
this document.

### 1.6 Type and range validation

Values are validated against the type and range in §3's table at
startup. An out-of-range value is a startup error with the same
loudness policy as §1.5. Callers downstream of the parser receive
strongly-typed, pre-validated values; no module re-validates.

---

## 2. Top-level vs sectioned keys

Keys are grouped as follows:

- **Top level** — cross-cutting settings that do not belong to a
  single module: `log_path`, `log_level`, `manifest_dir`,
  `reanalysis_default`. These names are already cited verbatim in
  `docs/STYLE.md` §4 and `docs/BACKLOG.md` (S0-19, S0-21); keeping
  them top-level preserves the references.
- **`[ingest]`, `[detect]`, `[focus]`** — module-scoped settings. A
  module sees only its own table plus any top-level keys it genuinely
  needs; the parser hands each module a strongly-typed config struct
  (see `docs/ARCHITECTURE.md` §6 "Every module → tomlplusplus config
  indirectly"). Modules do not parse TOML themselves.

No key is duplicated between top-level and a table. A future module
adding config ships a new table (`[species]`, `[feedback]`, …) rather
than mixing its keys into an existing one.

---

## 3. Field catalog

Every user-tunable value in the MVP. Ranges are inclusive unless
marked otherwise. "Locked by" identifies the task whose PR freezes the
default — `S0-18` means this file; others point at the task that will
adjust the default (and may also widen the range) inside its own PR.

### 3.1 Top-level

| Key | Type | Range / allowed values | Default | Satisfies | Locked by | Purpose |
|---|---|---|---|---|---|---|
| `log_path` | String (path) | any writable filesystem path | `~/Library/Logs/Wildframe/wildframe.log` | FR-5, FR-11, NFR-5 | S0-19 | spdlog daily-rotating file sink location (see `docs/STYLE.md` §4.3). Parent directories are created on first run. |
| `log_level` | String (enum) | `trace`, `debug`, `info`, `warn`, `error`, `critical` | `info` in `release` build, `debug` in `debug` / `asan` | NFR-5 | S0-18 | Threshold for both stdout and file sinks. Overrides the build-default in `docs/STYLE.md` §4.2 for field debugging. `trace` requires a debug/asan build — see §4. |
| `manifest_dir` | String (path) | any writable filesystem path | `~/Library/Application Support/Wildframe/batches/` | FR-5, FR-11, NFR-5 | S0-19 | Directory where per-batch JSON manifests (M6-05) are written. Each run creates `<ISO-8601-timestamp>.json` inside. Parent directories are created on first run. |
| `reanalysis_default` | String (enum) | `prompt`, `skip`, `overwrite` | `prompt` | FR-10, FR-11 | S0-21 | Default re-analysis behavior when existing `wildframe:*` sidecars are found. `prompt` shows the GUI dialog (M7-10); `skip` and `overwrite` run headless. |

### 3.2 `[ingest]`

| Key | Type | Range | Default | Satisfies | Locked by | Purpose |
|---|---|---|---|---|---|---|
| `max_depth` | Integer | `[1, 16]` | `1` | FR-1, FR-11 | M1-02 | Maximum directory-walk depth relative to the selected directory, following `find -maxdepth` semantics: `1` = files directly in the selected directory only, `2` = one level of subdirectories, etc. Upper bound prevents accidental disk-wide walks on misrooted configs. |

### 3.3 `[detect]`

| Key | Type | Range / allowed values | Default | Satisfies | Locked by | Purpose |
|---|---|---|---|---|---|---|
| `model` | String (enum) | `yolov11`, `megadetector` | `yolov11` | FR-3, FR-11 | M3-06 | Detector family. Both emit the same `DetectionResult` contract (M3-04). Recorded in provenance as `detector_model_name` using the canonical spelling (§6). |
| `execution_provider` | String (enum) | `coreml`, `cpu` | `coreml` | FR-3, FR-11 | M3-01 | ONNX Runtime execution provider. `coreml` with CPU fallback is the handoff §5 default; `cpu` forces CPU for deterministic benchmarking. Recorded in provenance as `execution_provider` (§6). |
| `confidence_threshold` | Real | `[0.0, 1.0]` | `0.25` | FR-3, FR-11, NFR-5 | M3-03 | Minimum per-box confidence retained after postprocessing. Boxes below this are dropped before NMS. Written verbatim into `wildframe_provenance:detection_confidence_threshold` (see `docs/METADATA.md` §4). |
| `iou_threshold` | Real | `[0.0, 1.0]` | `0.45` | FR-3, FR-11, NFR-5 | M3-03 | Non-max-suppression IoU threshold. Boxes whose IoU with a higher-scoring box exceeds this are dropped. Written verbatim into `wildframe_provenance:detection_iou_threshold`. |

### 3.4 `[focus]`

All `[focus]` defaults are **provisional** in S0-18. The M4 tasks that
own the algorithms (M4-02 for the Laplacian mapping, M4-05 for the
keeper formula) are the PRs that freeze the numeric values; each of
those PRs updates this table and `data/config.default.toml` in the
same commit. S0-18 ships placeholders only so a user who copies the
default file today sees every key that M4 will later interpret.

| Key | Type | Range | Default (provisional) | Satisfies | Locked by | Purpose |
|---|---|---|---|---|---|---|
| `laplacian_saturation` | Real | `> 0.0` | `1000.0` | FR-4, FR-11 | M4-02 | Saturation point for the Variance-of-Laplacian → `[0.0, 1.0]` mapping. Variance at or above this value maps to `1.0`; below it, the mapping is linear. Higher value = stricter "sharp" definition. |
| `edge_clipping_tolerance_pixels` | Integer | `[0, 64]` | `8` | FR-4, FR-11 | M4-04 | Pixel distance within which a bbox edge is considered to "touch" the corresponding image edge (see `docs/METADATA.md` §3.2). Measured against the preview image, not the original RAW. |
| `[focus.keeper_weights]` table | Real | each in `[0.0, 1.0]`; `focus + motion_blur + subject_size = 1.0` | see sub-keys | FR-4, FR-11 | M4-05 | Weights for the keeper-score combination formula (M4-05). Positive components sum to 1.0 so `keeper_score` stays in `[0.0, 1.0]` before penalties. |
| `focus.keeper_weights.focus` | Real | `[0.0, 1.0]` | `0.4` | FR-4 | M4-05 | Weight on `focus_score` (Variance-of-Laplacian, M4-02). |
| `focus.keeper_weights.motion_blur` | Real | `[0.0, 1.0]` | `0.3` | FR-4 | M4-05 | Weight on `motion_blur_score` (FFT high-frequency ratio, M4-03). |
| `focus.keeper_weights.subject_size` | Real | `[0.0, 1.0]` | `0.3` | FR-4 | M4-05 | Weight on `subject_size_percent` (M4-04). |
| `focus.keeper_weights.edge_clipping_penalty` | Real | `[0.0, 1.0]` | `0.2` | FR-4 | M4-05 | Penalty subtracted per clipped edge (M4-04). Applied after the weighted sum; result is clamped to `[0.0, 1.0]`. |

---

## 4. `trace` level and build interaction

`log_level = "trace"` is only effective when the binary was compiled
with `SPDLOG_ACTIVE_LEVEL = SPDLOG_LEVEL_TRACE` — i.e. the `debug` and
`asan` CMake presets. In a `release` build, trace-level log statements
are eliminated at compile time (`docs/STYLE.md` §4.2); setting
`log_level = "trace"` in that build is accepted by the parser but has
no effect beyond the `debug` level's behavior. Wildframe logs one
`warn` line at startup if this situation is detected so users know
why their trace lines are absent.

---

## 5. Error handling at startup

The parser's behavior on malformed input:

| Condition | Behavior |
|---|---|
| `--config <path>` names a nonexistent file | Startup error — the user asked for a specific file. |
| TOML syntax error | Startup error with line/column. |
| Unknown top-level or in-table key | Startup error naming the key (§1.5). |
| Value fails type check (e.g. `confidence_threshold = "high"`) | Startup error naming key, expected type, actual value. |
| Value fails range check (e.g. `confidence_threshold = 1.5`) | Startup error naming key, expected range, actual value. |
| `focus.keeper_weights` sub-keys do not sum to `1.0` within tolerance | Startup error naming the three weights and their sum. Tolerance is `1e-6`. |
| Partial file missing a key that has a default | Silent — default applies (§1.4). |
| Partial file missing a key with no default | Startup error — every required key in §3 has a default, so this is reserved for future required-without-default additions. |
| `log_path` or `manifest_dir` contains `~` but `$HOME` is unset or empty | Startup error naming the key — Wildframe cannot write its audit trail to an unresolved path (S0-19). |

"Startup error" means: Wildframe refuses to run, prints a one-line
diagnostic to stderr (this is the one place stderr is used; see
`docs/STYLE.md` §4.3), and exits with a non-zero status. The GUI
surfaces the same diagnostic in a modal before exiting.

Fail-loud is deliberate. A misconfigured detector threshold silently
defaulting would corrupt a batch of 500 sidecars whose provenance
fields then record the wrong threshold.

---

## 6. Relationship to provenance

Three `[detect]` keys are surfaced verbatim in every sidecar's
`wildframe_provenance:*` namespace (see `docs/METADATA.md` §4):

- `confidence_threshold` → `wildframe_provenance:detection_confidence_threshold`
- `iou_threshold` → `wildframe_provenance:detection_iou_threshold`
- `execution_provider` → `wildframe_provenance:execution_provider`

The config accepts the lowercase form (`coreml`, `cpu`); provenance
records the canonical brand spelling (`CoreML`, `CPU`). The mapping
is one-way and lives in `wildframe_detect`; users do not need to
write `CoreML` to get `CoreML` in their sidecars.

`model` is surfaced as `wildframe_provenance:detector_model_name`
with the canonical spelling (`YOLOv11`, `MegaDetector`), mapped the
same way.

---

## 7. Change policy

This document follows the same severity taxonomy as
`docs/METADATA.md` §9, scoped to config keys:

| Change | Severity | What it requires |
|---|---|---|
| **Add** a new key with a default | Non-breaking | Update this doc + `data/config.default.toml` + owning module in one PR. Pre-existing user configs keep working — the new key defaults in. |
| **Rename** an existing key | Breaking | Plan-change PR per `CONTRIBUTING.md`. Migrator reads the old key, emits a deprecation warning, and writes the new key. One release cycle later, remove the old-key reader. |
| **Retype** a key | Breaking | Same as rename. |
| **Tighten** a range | Breaking if any plausible existing config violates the tighter range; otherwise non-breaking. | Plan-change PR if breaking. |
| **Loosen** a range | Non-breaking | Same-PR doc + implementation update. |
| **Change a default** | Non-breaking in form; user-visible in effect. | Same-PR doc + `data/config.default.toml` + one-line note in the PR description describing the behavior change (CLAUDE.md §4.7). |
| **Remove** a key | Breaking | Plan-change PR. Reader treats the key as "absent and rejected"; removal of the underlying behavior is a separate concern. |

---

## 8. When to update this document

Update this document in the same PR as any of the following:

- Any change to `data/config.default.toml` — even a default-value tweak.
- Any change to a module's public `*Config` struct that the parser
  populates (`DetectConfig`, `FocusConfig`, future `IngestConfig`).
- A new key being added by a module task (M1-02, M3-*, M4-*).
- The "Locked by" column finalizes — when the named task lands, its
  PR flips the "(provisional)" note and pins the real default.

Stale config documentation is worse than absent documentation: users
write config files from this catalog, and a drifted catalog turns
their files into silent misconfigurations at the next upgrade.
