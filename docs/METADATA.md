# Wildframe Metadata Schema

This document is the **contract** that external tools rely on when
reading Wildframe-written XMP metadata. Lightroom smart collections,
Photo Mechanic filters, `exiftool` pipelines, and Wildframe's own
review UI all key on the field names, types, and value ranges
specified here.

Stability matters. Once a Wildframe release is in a user's hands, any
XMP field in this document may be present in sidecars that outlive
the installed binary. Renaming, retyping, or repurposing a field is
therefore a **breaking change**, governed by §9 below.

This document is the companion to handoff §13 (Metadata Schema). Where
the two disagree, the **handoff doc wins** — flag the drift per
`CLAUDE.md` §5 rather than editing either file to paper over it.
Implementation-side, `libs/metadata` (M5-05..M5-11) mirrors this
document exactly.

**Scope.** MVP only — the three `wildframe*` namespaces plus the
deterministic EXIF fields Wildframe mirrors for convenience. Phase 2+
namespaces (`wildframe_species:*`, `wildframe_feedback:*`) are listed
under §10 but not specified.

---

## 1. Storage

### 1.1 Sidecar file

Wildframe writes exactly one XMP sidecar per analyzed RAW file. The
sidecar lives next to the RAW file and takes its name by appending
`.xmp` to the full original filename **including the RAW extension**:

```
IMG_0421.CR3
IMG_0421.CR3.xmp    <- Wildframe sidecar
```

This `.CR3.xmp` convention (rather than `.xmp` replacing the
extension) matches Adobe Bridge, Lightroom Classic, and `exiftool`
defaults for RAW sidecars. Tools that write alongside the RAW —
Lightroom's own develop settings, for example — use the same
convention; Wildframe must coexist with those writes by preserving
unknown XMP nodes (see §1.3).

### 1.2 Format

Exiv2's high-level XMP API, serialized as RDF/XML. Hand-assembled XMP
is forbidden (handoff §15, risks). Write is atomic via temp-file plus
rename (M5-08); a partial write never leaves a torn sidecar on disk.

### 1.3 Coexistence with other writers

The writer preserves every XMP node it does not own. In particular:

- All nodes in namespaces Wildframe did not register are passed
  through untouched on read-modify-write.
- Nodes in the three `wildframe*` namespaces are fully owned by
  Wildframe: a write may delete, replace, or add any field within
  those namespaces without constraint.

### 1.4 JSON batch manifest

Wildframe also writes a per-batch JSON manifest (M6-05) covering every
image processed in a run. The manifest is an **append-only run log**,
not the source of truth — handoff §13 and FR-5 are explicit that
**the XMP sidecars are authoritative**. The manifest's structure is
documented where it is implemented, not here. A single pipeline run
emits one manifest row per image plus one run-level header row
(timing, pipeline version, detector model version).

---

## 2. XMP namespace declarations

Wildframe registers three custom XMP namespaces at startup (M5-05,
M5-06, M5-07). Each prefix is paired with a globally unique URI; the
URI is the stable identifier that survives even if a prefix is
renamed downstream.

| Prefix | Namespace URI | Written by | Purpose |
|---|---|---|---|
| `wildframe` | `https://wildframe.app/ns/ai/1.0/` | Pipeline (AI) | Detection + focus/keeper scores. |
| `wildframe_provenance` | `https://wildframe.app/ns/provenance/1.0/` | Pipeline (AI) | Model name/version, pipeline version, timestamps. |
| `wildframe_user` | `https://wildframe.app/ns/user/1.0/` | Review UI (user) | Human overrides and notes. |

### 2.1 URI stability (**read this before changing a URI**)

The URI is part of the persistent record. Once any sidecar on disk
cites a URI, that URI is the identifier downstream tools will use to
find our fields — Lightroom smart collection rules, `exiftool`
plugins, and future Wildframe versions all look up fields by
`(URI, local name)`, not by prefix.

Consequences:

- **Changing a URI is a breaking change** for every sidecar ever
  written. Treat it as a Wildframe major-version bump (`v1.x → v2.0`)
  and ship a one-shot migrator that rewrites old sidecars in place.
- **The `1.0/` segment is intentional.** If a breaking schema change
  is unavoidable, the new schema ships under `.../2.0/` and the
  migrator rewrites existing sidecars. Non-breaking additions (new
  fields, new optional sub-fields) stay on `.../1.0/`.
- **URIs do not need to resolve.** XMP URIs are identifiers, not URLs
  the XMP parser dereferences. `wildframe.app` does not need to host
  anything at these paths.

### 2.2 Prefix collisions

If a user's existing XMP tooling happens to use the prefix
`wildframe` for a different URI, the XMP RDF model disambiguates by
URI: two namespaces with the same prefix but different URIs are
distinct. Wildframe's writer always emits its prefixes paired with
the URIs in §2, so collisions do not corrupt other tools' data.

---

## 3. `wildframe:*` — AI detection + scoring

Written by the pipeline (`wildframe_metadata::write_sidecar`, M5-08).
Read by `wildframe_metadata::read_sidecar` (M5-09) and by the GUI
filter controls (M7-06). Never written by the review UI — that is
§5's job.

The XMP type column uses Adobe XMP type names (Boolean, Integer,
Real, Text, Date, Seq, Bag, Struct). Ranges given as `[a, b]` are
inclusive.

| Field | XMP type | Range / units | Required | Purpose |
|---|---|---|---|---|
| `bird_present` | Boolean | `True` / `False` | Required | Whether the primary detector returned at least one bird-class box above the confidence threshold. |
| `bird_count` | Integer | `[0, N]` | Required | Number of bird detections above the confidence threshold. |
| `bird_boxes` | Seq of Struct (see §3.1) | 0..N entries | Required (may be empty) | All bird-class detections that passed confidence + NMS. Ordering: sorted by descending `confidence`. |
| `primary_subject_index` | Integer | `[0, bird_count-1]` | Required if `bird_count > 0`; absent otherwise | 0-based index into `bird_boxes` identifying the primary subject (max confidence; ties broken by bbox area — FR-3). |
| `detection_confidence` | Real | `[0.0, 1.0]` | Required if `bird_count > 0`; absent otherwise | Maximum per-box confidence across `bird_boxes`. |
| `focus_score` | Real | `[0.0, 1.0]` | Required if `bird_count > 0`; absent otherwise | Normalized Variance-of-Laplacian on the primary subject crop (FR-4, M4-02). Higher = sharper. |
| `motion_blur_score` | Real | `[0.0, 1.0]` | Required if `bird_count > 0`; absent otherwise | FFT high-frequency energy ratio on the primary subject crop (FR-4, M4-03). Higher = less motion blur. |
| `subject_size_percent` | Real | `[0.0, 1.0]` | Required if `bird_count > 0`; absent otherwise | Primary subject bbox area divided by preview image area (FR-4, M4-04). |
| `edge_clipping` | Struct (see §3.2) | — | Required if `bird_count > 0`; absent otherwise | Per-edge flags indicating whether the primary subject bbox touches that edge of the preview within tolerance (FR-4, M4-04). |
| `keeper_score` | Real | `[0.0, 1.0]` | Required | Weighted combination of focus / motion-blur / subject size with an edge-clipping penalty (FR-4, M4-05). When `bird_present = False`, this is `0.0` by definition. |

### 3.1 `bird_boxes` entry structure

Each entry in the `bird_boxes` Seq is an XMP Struct with four Real
fields and one Real confidence:

| Sub-field | XMP type | Range / units | Purpose |
|---|---|---|---|
| `x` | Real | `[0.0, 1.0]` | Left edge of the bbox, as a fraction of preview image width. |
| `y` | Real | `[0.0, 1.0]` | Top edge of the bbox, as a fraction of preview image height. |
| `w` | Real | `(0.0, 1.0]` | Bbox width as a fraction of preview image width. |
| `h` | Real | `(0.0, 1.0]` | Bbox height as a fraction of preview image height. |
| `confidence` | Real | `[0.0, 1.0]` | Per-box detection confidence returned by the model after postprocessing. |

**Coordinate system.** Origin at top-left of the preview, x-axis
rightward, y-axis downward. Values are normalized so sidecars stay
valid if the preview is later resized or re-extracted at a different
resolution. This is the same convention YOLOv11 and MegaDetector
emit internally after letterbox removal.

**Why normalized instead of pixels.** Pixel coordinates would tie
sidecars to the preview's pixel dimensions, which differ by camera
model and across LibRaw versions. Normalized coordinates let the GUI
draw the bbox correctly regardless of which resolution it chooses to
display.

### 3.2 `edge_clipping` structure

An XMP Struct with four Boolean fields, one per image edge:

| Sub-field | XMP type | Purpose |
|---|---|---|
| `top` | Boolean | Primary subject bbox touches the top edge within tolerance (M4-04). |
| `bottom` | Boolean | Same, bottom edge. |
| `left` | Boolean | Same, left edge. |
| `right` | Boolean | Same, right edge. |

The tolerance is a configurable pixel count resolved from the TOML
config (FR-11, S0-18) and recorded in §4's provenance.

---

## 4. `wildframe_provenance:*` — run provenance

Written by the pipeline at sidecar-write time (M5-08). Never written
by the review UI. These fields answer "what produced these AI values
and when" and are the basis for auditability (NFR-5).

| Field | XMP type | Format / range | Required | Purpose |
|---|---|---|---|---|
| `analysis_timestamp` | Date | ISO-8601 UTC (e.g. `2026-04-21T18:42:03Z`) | Required | When this sidecar's AI values were written. |
| `pipeline_version` | Text | semver — matches the shipped `wildframe` binary's version tag | Required | The Wildframe application version that wrote this sidecar. Independent of the model version. |
| `detector_model_name` | Text | `YOLOv11` or `MegaDetector` (extensible — see §10) | Required | Detector family used for this sidecar. |
| `detector_model_version` | Text | Upstream version string (e.g. `11.0.0`) **or** SHA256 hex from `tools/fetch_models.cmake` (S0-11) | Required | Pinpoints the exact weights used, for reproducibility and licensing traceability (handoff §15). |
| `focus_algorithm_version` | Text | semver of the `FocusConfig` + formula (M4-05) | Required | Bumps when the keeper-score formula or its weights change. Lets downstream comparisons distinguish "old formula" from "new formula" sidecars. |
| `detection_confidence_threshold` | Real | `[0.0, 1.0]` | Required | The confidence threshold applied during NMS for this run (FR-11). |
| `detection_iou_threshold` | Real | `[0.0, 1.0]` | Required | The IoU threshold applied during NMS for this run. |
| `execution_provider` | Text | `CoreML` or `CPU` | Required | ONNX Runtime execution provider used. Useful for reproducing results (handoff §5). |

**Why versions instead of hashes everywhere.** Upstream model
versions are human-readable; the SHA256 pinned by
`tools/fetch_models.cmake` is authoritative. The field accepts either
so older sidecars written before a hash was pinned remain valid.

---

## 5. `wildframe_user:*` — user overrides

Written by the GUI review UI (M7-07, via
`wildframe_metadata::write_user_override`, M5-10). The AI never
writes into this namespace; no user override ever modifies a
`wildframe:*` or `wildframe_provenance:*` field. This separation is
the mechanism by which FR-6 keeps AI-generated vs user-corrected
values distinguishable.

Every field here is optional and defaults to `Unset`.

| Field | XMP type | Allowed values | Purpose |
|---|---|---|---|
| `bird_present_override` | Text (closed enum) | `Present`, `Absent`, `Unset` | User correction to `wildframe:bird_present`. `Unset` means the user has not opined; filter predicates should fall back to the AI value. |
| `keeper_override` | Text (closed enum) | `Approved`, `Rejected`, `Unset` | User verdict on the frame. Drives the approve/reject toggle (M7-07). |
| `user_note` | Text | Free text, Unicode | User-provided note. No Wildframe-imposed length cap; consumers should expect multi-line strings. |
| `override_timestamp` | Date | ISO-8601 UTC | When the most recent field in this namespace was last changed. Updated on any write into `wildframe_user:*` (M5-10). |
| `reanalysis_policy_used` | Text (closed enum) | `PromptSkipped`, `PromptOverwrote`, `PerFile` | Records what the user did the *last* time this file was eligible for re-analysis (FR-10, S0-21). Absent if the file has never been subject to a re-analysis decision. |

### 5.1 Enum value stability

Enum values above are closed sets. Readers must treat any value
outside the allowed set as if the field were `Unset` (for the first
two enums) or as an unknown sentinel (for `reanalysis_policy_used`)
— never crash, never silently coerce.

### 5.2 Filter semantics (FR-6, FR-8)

When the GUI computes filters:

- `bird_present` for display/filter purposes is:
  `bird_present_override` if it is `Present` or `Absent`, else
  `wildframe:bird_present`.
- `keeper` approve/reject state is `keeper_override` verbatim;
  `Unset` means "no verdict yet" and is the default.

These rules live in the GUI (M7-06), but they depend on the
separation this namespace enforces.

---

## 6. Deterministic EXIF mirror (read-only)

Handoff §13 records a set of **deterministic EXIF fields** that
Wildframe reads from the RAW file into a `DeterministicMetadata`
struct (M5-01, M5-02). These values originate from the camera, not
from Wildframe's pipeline, so MVP does **not** write them into the
`wildframe:*` namespace or duplicate them in a Wildframe-owned XMP
node. Standard XMP / EXIF namespaces (`exif:`, `tiff:`,
`photoshop:`) already cover them, and Lightroom / Photo Mechanic /
`exiftool` already read them from the RAW directly.

The fields Wildframe reads but does not re-serialize:

- `file_path`, `file_name` (derived from the filesystem — not XMP).
- `capture_datetime` (`exif:DateTimeOriginal`).
- `camera_model` (`tiff:Model`).
- `lens_model` (`exifEX:LensModel` or `aux:Lens`).
- `focal_length` (`exif:FocalLength`).
- `aperture` (`exif:FNumber`).
- `shutter_speed` (`exif:ExposureTime`).
- `iso` (`exif:ISOSpeedRatings`).
- `gps_lat`, `gps_lon` (`exif:GPSLatitude`, `exif:GPSLongitude`).
- `image_dimensions` (`tiff:ImageWidth`, `tiff:ImageLength`).

If a future requirement demands a Wildframe-owned copy of any of
these (e.g. for offline manifest filtering without re-opening the
RAW), add it to `wildframe:*` via the change-policy process in §9 —
do not invent a new namespace.

---

## 7. Value conventions

Rules that apply across all three namespaces.

### 7.1 Numeric types

- **Real** values are serialized as decimal strings with enough
  precision to round-trip float32 (`%.9g` is sufficient). Readers
  must tolerate scientific notation.
- **Integer** values are serialized as decimal strings, no thousands
  separator, no leading zeros.
- **Normalized Reals** (`[0.0, 1.0]`) are never clamped lossily at
  write time — clamping is the responsibility of the producing
  algorithm (e.g. M4-02 clamps before writing). Values outside
  `[0.0, 1.0]` on read indicate a bug upstream; readers should log
  and clamp rather than propagating out-of-range values.

### 7.2 Timestamps

- All Date fields are ISO-8601 in **UTC**, suffix `Z`. No offsets,
  no local-time strings. Example: `2026-04-21T18:42:03Z`.
- Timestamps are written once per run — a single batch that
  processes 500 images records the same `analysis_timestamp` on
  every sidecar it writes in that run.

### 7.3 Enums

- Enum values are the exact ASCII strings listed in the tables above.
  Case-sensitive (`Present`, not `present` or `PRESENT`).
- Readers must treat unknown enum values as the documented fallback
  (§5.1).

### 7.4 Absence vs zero

A field that is "absent" is **not serialized** — the XMP parser sees
no node. Absence is not the same as the zero value:

- `bird_count = 0` means "we ran the detector and found no birds".
- `bird_count` absent means "we did not record a count" — a sidecar
  from a broken run, a partial file, or a pre-Wildframe tool.

Readers must distinguish these cases when filtering. Wildframe's own
filter predicates (M7-06) treat "absent" as "not yet analyzed".

---

## 8. Example sidecar

For a file `IMG_0421.CR3` analyzed by a hypothetical v0.1.0 release:

```xml
<x:xmpmeta xmlns:x="adobe:ns:meta/">
 <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
  <rdf:Description rdf:about=""
    xmlns:wildframe="https://wildframe.app/ns/ai/1.0/"
    xmlns:wildframe_provenance="https://wildframe.app/ns/provenance/1.0/"
    xmlns:wildframe_user="https://wildframe.app/ns/user/1.0/">

   <wildframe:bird_present>True</wildframe:bird_present>
   <wildframe:bird_count>1</wildframe:bird_count>
   <wildframe:bird_boxes>
    <rdf:Seq>
     <rdf:li rdf:parseType="Resource">
      <wildframe:x>0.312</wildframe:x>
      <wildframe:y>0.414</wildframe:y>
      <wildframe:w>0.186</wildframe:w>
      <wildframe:h>0.220</wildframe:h>
      <wildframe:confidence>0.873</wildframe:confidence>
     </rdf:li>
    </rdf:Seq>
   </wildframe:bird_boxes>
   <wildframe:primary_subject_index>0</wildframe:primary_subject_index>
   <wildframe:detection_confidence>0.873</wildframe:detection_confidence>
   <wildframe:focus_score>0.742</wildframe:focus_score>
   <wildframe:motion_blur_score>0.681</wildframe:motion_blur_score>
   <wildframe:subject_size_percent>0.041</wildframe:subject_size_percent>
   <wildframe:edge_clipping rdf:parseType="Resource">
    <wildframe:top>False</wildframe:top>
    <wildframe:bottom>False</wildframe:bottom>
    <wildframe:left>False</wildframe:left>
    <wildframe:right>False</wildframe:right>
   </wildframe:edge_clipping>
   <wildframe:keeper_score>0.705</wildframe:keeper_score>

   <wildframe_provenance:analysis_timestamp>2026-04-21T18:42:03Z</wildframe_provenance:analysis_timestamp>
   <wildframe_provenance:pipeline_version>0.1.0</wildframe_provenance:pipeline_version>
   <wildframe_provenance:detector_model_name>YOLOv11</wildframe_provenance:detector_model_name>
   <wildframe_provenance:detector_model_version>11.0.0</wildframe_provenance:detector_model_version>
   <wildframe_provenance:focus_algorithm_version>1.0.0</wildframe_provenance:focus_algorithm_version>
   <wildframe_provenance:detection_confidence_threshold>0.25</wildframe_provenance:detection_confidence_threshold>
   <wildframe_provenance:detection_iou_threshold>0.45</wildframe_provenance:detection_iou_threshold>
   <wildframe_provenance:execution_provider>CoreML</wildframe_provenance:execution_provider>

  </rdf:Description>
 </rdf:RDF>
</x:xmpmeta>
```

The user-override namespace is omitted above because no user has
reviewed the file yet. After the user marks it Approved with a note:

```xml
   <wildframe_user:keeper_override>Approved</wildframe_user:keeper_override>
   <wildframe_user:user_note>Frame for the portfolio — crop tight on the subject.</wildframe_user:user_note>
   <wildframe_user:override_timestamp>2026-04-22T09:15:07Z</wildframe_user:override_timestamp>
```

The AI namespace is untouched by the override write (FR-6, §5).

---

## 9. Change policy (stability contract)

### 9.1 What each kind of change requires

| Change | Severity | What it requires |
|---|---|---|
| **Add** a new field to an existing namespace | Non-breaking | Update this doc + `libs/metadata` + tests in one PR. No URI bump. |
| **Rename** an existing field | Breaking | Plan-change PR per `CONTRIBUTING.md`, URI bump to `.../2.0/`, one-shot migrator, Wildframe major-version bump. |
| **Retype** an existing field (e.g. Real → Text) | Breaking | Same as rename. |
| **Tighten** a value range or enum | Breaking if readers rely on the wider range | Plan-change PR. If values currently on disk remain valid under the tighter range, this is non-breaking; if any existing sidecar could violate the new range, it is breaking. |
| **Loosen** a value range | Non-breaking | Update this doc in the same PR as the implementation. |
| **Remove** a field | Breaking | Plan-change PR. Writers stop emitting; readers must treat it as "absent" (§7.4). Consider whether a migrator is needed. |
| **Change a namespace URI** | Breaking | Last-resort. URI bump + migrator + major-version bump (§2.1). |

### 9.2 Unconditional rules

- Never add a field that the pipeline *could* set but the review UI
  *also* sets — field ownership is strictly AI or user, per §4 vs §5.
- Never repurpose a field name under a new meaning. Old sidecars
  carrying the old value become silently wrong. Pick a new name.
- Never write into a namespace prefix you do not own. The Wildframe
  writer must not touch `exif:*`, `xmp:*`, `dc:*`, `lr:*`, or
  anything else produced by other tools (§1.3).

---

## 10. Out of scope (for MVP)

The following exist in the phased roadmap (handoff §14) but are
**not** specified here. When they land, each gets its own section in
this document alongside the existing three namespaces:

- `wildframe_species:*` (Phase 2) — top-k species predictions.
- `wildframe_feedback:*` (Phase 2) — training-ready label rows
  captured from user overrides.
- Any Lightroom-export-specific XMP (Phase 3) — emitted by
  `wildframe_lightroom` if it needs namespaces beyond the above.

MVP sidecars must not include fields from these namespaces. A reader
encountering one is reading a sidecar written by a newer Wildframe
and should ignore the unknown namespace (§1.3).

---

## 11. When to update this document

Update this document in the same PR as any of the following changes:

- Any edit to `libs/metadata`'s public headers that changes the
  field set, types, or value ranges (M5-01, M5-05..M5-07).
- A bump to `pipeline_version`, `focus_algorithm_version`, or the
  `FocusConfig` defaults that changes the meaning of a keeper score.
- Pinning the final namespace URIs in M5-05. The URIs in §2 are
  customer-review candidates; the first M5-05 PR is the last chance
  to change them without triggering §9's breaking-change process.
- Adding a new namespace or field per §9.

Stale schema documentation is worse than absent schema
documentation. If this document drifts from what `libs/metadata`
actually writes, flag it per CLAUDE.md §5 rather than silently
fixing one side.
