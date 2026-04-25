# Wildframe Licensing

This document records the licensing posture of Wildframe: its own
license (GPL-3.0-or-later), the obligations inherited from Qt 6's LGPL
choice, the upstream licenses of every statically linked dependency, and
the upstream licenses of the ONNX model weights fetched at build time.

It is reviewed as part of Definition of Done whenever a new dependency is
added to `vcpkg.json` or when `tools/fetch_models.cmake` (S0-11) pins a new
model URL. Handoff §15 ("Risks and Design Cautions") calls out licensing as
a project risk; this file is the standing response to that risk.

**This document is not legal advice.** It is a developer-maintained summary
intended to make the licensing posture legible. Before a public release,
the customer should obtain qualified legal review of the combined work.

---

## 1. Wildframe's own license

**Decision: GPL-3.0-or-later.** Wildframe is free and open-source
software distributed under the GNU General Public License, version 3.0
or (at your option) any later version. SPDX identifier:
`GPL-3.0-or-later`.

### Why GPL-3.0-or-later

The choice is constrained by the licenses of statically linked upstream
dependencies (see §4). Exiv2 is distributed under GPL-2.0-or-later and
is the strongest copyleft in the dependency graph; because it is
statically linked into the shipped binary, the combined work must be
distributed under a GPL-compatible license. Among the GPL-compatible
options:

- **GPL-3.0-or-later** (chosen) — compatible with every current
  dependency, including Apache-2.0 (OpenCV) which is compatible with
  GPL-3.0 but *not* GPL-2.0-only, and LGPL-3.0 (Qt 6). The `or-later`
  clause preserves flexibility if future GPL versions address new
  concerns.
- **GPL-2.0-or-later** — also compatible, but less future-flexible and
  loses Apache-2.0 compatibility on the GPL-2.0-only branch.
- **Proprietary / commercial** — would require replacing Exiv2 with a
  non-copyleft alternative and purchasing a commercial Qt license. Not
  aligned with the customer's intent to release as FOSS.
- **Permissive (MIT / Apache-2.0 / BSD)** — cannot be applied to a
  binary that statically links GPL Exiv2 code; the combined distribution
  would still be effectively GPL regardless of Wildframe's own
  declaration.

### Follow-on work (not part of S0-08)

The decision is recorded here, but two artifacts still need to be
created to make it machine-readable and user-visible — those are not in
scope for S0-08 and should be tracked as a follow-up backlog item:

- Add a `LICENSE` file at the repo root containing the full
  GPL-3.0-or-later license text.
- Update `vcpkg.json` to set `"license": "GPL-3.0-or-later"` (currently
  `null`).

Until those land, this document is the authoritative record of the
license choice.

---

## 2. Qt 6 — LGPL dynamic-linking obligation

Qt 6 is the only dependency that is **dynamically linked** in the shipped
binary. This is a deliberate choice driven by Qt's LGPL-3.0 licensing
(handoff §15 and decision 6 in §16).

### Why dynamic linking matters

LGPL-3.0 permits linking a proprietary or differently licensed application
against LGPL libraries **only if** the end user retains the ability to
replace the LGPL library with a modified version. In practice, for a C++
desktop application this is satisfied by:

1. **Dynamic linking** against Qt's shared libraries (so the user can swap
   in a modified Qt by replacing the `.dylib` / framework bundle).
2. **Shipping Qt's source** (or a written offer to provide it) for the
   exact version used.
3. **Shipping the LGPL-3.0 license text** alongside the application.
4. **Attribution**: preserving Qt's copyright notices in about/credits UI
   and in the distributed bundle.

Static linking Qt 6 is **forbidden** under this posture — it would require
a commercial Qt license from The Qt Company.

### Operational rules

- `qtbase` is the only vcpkg port for which dynamic linking is mandatory
  in the shipped macOS bundle. All other vcpkg ports are built and linked
  statically per the handoff's module layout.
- Release packaging (future work — not scoped in Sprint 0) must:
  - Bundle Qt frameworks next to the Wildframe executable in the `.app`
    structure, resolvable via `@rpath`.
  - Include `qt_source.tar.xz` or a clearly documented URL + commit for
    the exact Qt version in `vcpkg.json`.
  - Include `third_party_licenses/qt-lgpl-3.0.txt`.
- Any future change to link Qt statically **must** be gated on
  customer-approved commercial Qt licensing and a plan-change PR per
  `CONTRIBUTING.md`.

---

## 3. Model weights — fetched at build time, not committed

Model weights are **never committed to this repository**.
`tools/fetch_models.cmake` (S0-11) downloads them from pinned URLs during
CMake configure, verifies SHA256, and caches them under `build/_models/`.

Some pinned URLs point upstream directly; others point to a GitHub Release
attached to this repository that hosts a Wildframe-produced export (for
example, an ONNX file we produced from an upstream `.pt` via the upstream
exporter). In either case the model file itself never enters git history.
Hosting a Wildframe-produced export on our own GitHub Releases is a form
of redistribution, so the upstream license's obligations attach to
Wildframe's distribution whenever we self-host an export of an
otherwise-copyleft weight.

### YOLOv11 (default detector — FR-3)

- **Upstream:** Ultralytics (https://github.com/ultralytics/ultralytics).
- **Upstream license (code + published weights):** AGPL-3.0-only, with a
  separate commercial license offered by Ultralytics.
- **Delivery:** Ultralytics does not publish a canonical ONNX asset;
  upstream ships `.pt` weights only. Wildframe exports to ONNX via the
  upstream exporter and hosts the result as a release asset on this
  repository. Export command:
  ```
  YOLO('yolo11n.pt').export(
      format='onnx', imgsz=640, opset=17,
      simplify=True, dynamic=False, nms=False)
  ```
  Exporter toolchain: ultralytics 8.4.41, torch 2.2.2, onnx 1.21.0,
  onnxslim 0.1.91. NMS is deliberately *not* baked into the ONNX — the
  detect module applies confidence thresholding and NMS in C++ (M3-03).
- **Redistribution posture:** Wildframe redistributes the exported ONNX
  under AGPL-3.0 via its GitHub Release. Wildframe's own license is
  GPL-3.0-or-later (§1), which is compatible with AGPL-3.0, so the
  combined source-plus-asset distribution is consistent.
- **Downstream obligation at runtime:** the combined work (Wildframe
  binary + AGPL weights on disk) may create AGPL obligations if a
  downstream user then redistributes that combined work, including
  network redistribution. This is a consideration for redistributors
  of built binaries, not for users running Wildframe locally.
- **Pinned URL:** `https://github.com/mroy113/wildframe/releases/download/models-v1/yolo11n.onnx`
- **Pinned SHA256:** `119040d6483aee48c168b427bf5d383d6cadde5745c6991e6dc24eeabf07a05a`
- **Size:** 10,741,340 bytes.
- **Re-export procedure:** re-run the export command above against a
  newer Ultralytics release, attach the new `yolo11n.onnx` to a new
  release tag (`models-v2`, `models-v3`, …; release tags are immutable),
  and update both the URL and the SHA256 in `tools/fetch_models.cmake`
  and in this section in the same PR.

If the customer wants a detector with lighter downstream obligations, the
MegaDetector alternative path (M3-06) exists precisely for that purpose.

### MegaDetector (alternative detector — FR-3, handoff §15 mitigation)

- **Upstream:** Microsoft AI for Earth / current maintainers
  (https://github.com/microsoft/CameraTraps /
  https://github.com/agentmorris/MegaDetector).
- **License:** MIT for the MegaDetector codebase and for MDv5 weights
  (MDv5 is a YOLOv5-derived model whose weights are released by Microsoft
  under MIT terms). **MDv6** is built on top of Ultralytics YOLOv6/v8/v9
  internals and its licensing therefore inherits the upstream Ultralytics
  AGPL-3.0 posture. Confirm the MegaDetector *version* before pinning.
- **Redistribution terms:** MIT-licensed weights can be freely
  redistributed with attribution. Wildframe still does not bundle them;
  they are fetched at build time for consistency with YOLOv11.
- **Pinned URL + SHA256:** to be locked in during S0-11, alongside the
  YOLOv11 entry. Update this section when the alternative path actually
  wires up in M3-06.

### Do not commit weights

Committing any `.onnx`, `.pt`, `.safetensors`, or other model-weight file
to this repository is forbidden, independently of that weight's license.
`.gitignore` already excludes `build/_models/`. CI should fail any PR
that attempts to introduce a large binary under that tree.

---

## 4. Statically linked dependency matrix

All versions below are mirrored from `vcpkg.json` (the authoritative
source). When you bump a version, update the **version** column here in
the same PR.

| Dependency | Version | Upstream license | Notes |
|---|---|---|---|
| **libraw** | 0.22.0 | LGPL-2.1-or-later **OR** CDDL-1.0 (dual) | LGPL static-linking caveats do not apply when the binary is distributed under a GPL-compatible license (Wildframe's likely posture, §1). |
| **exiv2** | 0.28.8 (`xmp` feature) | GPL-2.0-or-later | **Strongest copyleft in the tree.** Static linking propagates GPL to the combined binary. This is the dominant constraint on §1. The `xmp` feature is enabled because `wildframe_metadata` uses Exiv2's XMP API to read/write sidecars (TB-07, M5-*); without it, `XmpParser::encode/decode` are stubs that fail at runtime. |
| **expat** | transitive via exiv2[xmp] | MIT | Pulled in by Exiv2's `xmp` feature — the XMP toolkit bundled with Exiv2 uses expat to parse RDF/XML packets. Permissive, no reciprocal obligation. |
| **opencv4** | 4.12.0 (port-version 1) | Apache-2.0 | Modern OpenCV (≥ 4.5.0) is Apache-2.0. Compatible with GPL-3.0 distribution; **not** compatible with GPL-2.0-only (use `or-later`). |
| **onnxruntime** | 1.23.2 | MIT | Permissive, no reciprocal obligation. |
| **qtbase** | 6.10.2 | LGPL-3.0-only (with commercial alternative) | **Dynamically linked** per §2. Not included in the "statically linked" compatibility analysis above. |
| **nlohmann-json** | 3.12.0 (port-version 2) | MIT | Permissive. |
| **tomlplusplus** | 3.4.0 (port-version 1) | MIT | Permissive. Not called out in S0-08's original dependency list but present in `vcpkg.json` (TOML runtime config, handoff §5). |
| **spdlog** | 1.17.0 | MIT | Permissive. Bundles `fmt` by default; fmt is MIT as well. |
| **gtest** | 1.17.0 (port-version 2) | BSD-3-Clause | Permissive. **Test-only dependency** — linked into `*_test` targets, never into the shipped `wildframe` executable. Its license therefore does not constrain the shipped binary. |

### Implications for Wildframe's binary license (summary)

- **GPL-2.0-or-later** from Exiv2 dominates.
- **Apache-2.0** from OpenCV is compatible with GPL-3.0 (but not
  GPL-2.0-only — hence the "or-later" preference in §1).
- **LGPL-2.1-or-later** from LibRaw is satisfied by any GPL-compatible
  distribution.
- **MIT / BSD** from the remaining permissive dependencies impose only
  attribution obligations.
- **LGPL-3.0** from Qt 6 is satisfied by the dynamic-linking posture in §2.

The shipped binary's effective license is therefore **GPL-3.0-or-later**
unless §1 is resolved differently *and* Exiv2 is replaced.

---

## 5. Attribution bundle (for distribution)

When Wildframe is packaged for distribution (post-MVP; tracked outside
Sprint 0), the bundle must include a `third_party_licenses/` directory
with, at minimum:

- The full license text for each dependency in §4 and §2.
- The full license text for each model weight in §3, if the user opts to
  pre-bundle weights with their own distribution (Wildframe's own
  distribution does not).
- A top-level `NOTICE` or `CREDITS` file enumerating each dependency and
  its upstream URL.
- For Qt specifically: the LGPL-3.0 text plus either the Qt source
  tarball or a written offer for the source (see §2).

Vcpkg produces a `copyright` file under
`vcpkg_installed/<triplet>/share/<port>/` for each port; the packaging
step can harvest these rather than maintaining license texts by hand.

---

## 6. When to update this document

Update this document in the same PR as any of the following changes:

- Adding, removing, or version-bumping any entry in `vcpkg.json`
  (refresh §4).
- Landing or modifying `tools/fetch_models.cmake` (refresh §3 with the
  pinned URLs and SHA256s).
- Adding the `LICENSE` file at the repo root and updating the
  `"license"` field in `vcpkg.json` to `"GPL-3.0-or-later"` per §1.
- Any change that moves a dependency from static to dynamic linking or
  vice versa (§2 is the template for a dynamic-linking section).
- Release packaging work that builds the attribution bundle in §5.

Stale licensing information is worse than absent licensing information.
If this document drifts from `vcpkg.json` or from the shipped binary's
actual composition, flag it per CLAUDE.md §5 rather than silently fixing.
