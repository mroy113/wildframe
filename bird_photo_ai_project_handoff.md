---
document_type: project_handoff
project_name: bird-photo-ai-metadata-engine
status: concept_and_mvp_planning
primary_goal: >
  Build an AI-assisted application that analyzes RAW bird photos, generates useful metadata,
  and integrates with an existing photo workflow, likely Lightroom Classic.
intended_recipient: software_project_planning_agent
source_context: >
  Derived from user discussion about bird-photo culling, RAW workflows, Lightroom/DxO/Photo Mechanic,
  and interest in extending metadata beyond culling to species and related AI-enriched annotations.
---

# Project Handoff: AI-Assisted Bird Photo Metadata and Culling System

## 1. Executive Summary

Build a system that analyzes folders or selections of RAW bird photographs and writes back useful metadata for culling, search, and later workflow automation.

The initial user need is not generic photo editing. The initial user need is:

1. Detect whether a bird is present in the frame.
2. Determine whether the bird is in focus.

The broader product direction is to treat image analysis as a metadata enrichment pipeline, not just a pass/fail culling tool. Once the system can reliably detect and crop birds, it should be able to attach richer metadata such as likely species, bird count, action/context tags, and image quality scores.

The most practical implementation is:

- a standalone analysis engine for computer vision / AI
- plus a thin integration layer for an existing photo workflow application, most likely Lightroom Classic

This keeps the AI stack flexible while still supporting real-world photo workflows.

---

## 2. Product Vision

### Core product idea

An AI-assisted wildlife photo intelligence tool focused first on birds.

### Initial value proposition

Given a set of RAW photos, automatically infer structured metadata that helps with:

- culling
- filtering
- search
- ranking
- species-oriented organization

### Long-term value proposition

Transform large bird-photo libraries into searchable, metadata-rich collections that support queries like:

- show me sharp in-flight photos
- show me likely warblers from this trip
- show me all high-confidence osprey photos over water
- show me the best image from each burst
- show me photos that likely match an eBird checklist in place/time

---

## 3. Scope

## In scope for MVP

- Analyze a folder or selected set of RAW images
- Detect whether a bird is present
- Produce at least one bird bounding box when a bird is present
- Estimate whether the primary bird subject is in focus
- Produce machine-readable metadata outputs
- Support manual review of outputs
- Write metadata to sidecar or catalog-compatible structures
- Support later host-app integration, likely Lightroom Classic

## Out of scope for MVP

- Full image editing
- Full DAM/catalog replacement
- Perfect species identification
- Fine-grained age/sex/plumage classification
- Guaranteed scientific correctness
- Real-time in-camera operation
- Full mobile app support

---

## 4. Primary User Goals

### User goals

- Reduce time spent manually culling large burst-heavy bird photo sets
- Avoid keeping obvious non-bird frames
- Surface likely sharp keepers faster
- Add searchable species and context metadata
- Preserve compatibility with existing RAW editing workflow

### Workflow assumptions

- User likely works with RAW bird photos in bulk
- User may already use Lightroom, DxO, or other desktop editing tools
- User values metadata enrichment beyond simple keep/reject
- User is technically capable and may build custom software or plugins

---

## 5. High-Level Requirements

## Functional requirements

### FR-1: Input handling
- System must ingest folders or selected image sets containing RAW files.
- System should support common bird-photography RAW formats, starting with Canon CR3/CR2 and extensible to other formats.
- System should preserve original image files and avoid destructive changes.

### FR-2: Bird presence detection
- System must determine whether one or more birds are present in an image.
- System should output bounding boxes for detected birds.
- System should distinguish between no bird, likely bird, and high-confidence bird.

### FR-3: Focus assessment
- System must estimate whether the primary bird subject is acceptably in focus.
- System should score subject sharpness based on the bird crop rather than whole-image sharpness.
- System should prefer bird/head/eye sharpness over global image quality.

### FR-4: Metadata generation
- System must generate structured metadata for each image.
- Metadata should include at minimum:
  - bird_present
  - bird_count
  - bird_boxes
  - primary_subject_focus_score
  - keeper_score
- System should support richer metadata such as:
  - species top-k predictions
  - confidence values
  - action/context tags
  - motion blur score
  - obstruction score
  - eye-visible flag
  - subject-size estimate

### FR-5: Output persistence
- System must persist metadata in a form usable by downstream tools.
- System should support one or more of:
  - XMP sidecar output
  - Lightroom custom metadata
  - JSON manifest per batch
  - SQLite/Postgres metadata store for internal app use

### FR-6: Review and override
- System should allow human review and correction of model outputs.
- System should allow manual override of species or quality results.
- System should preserve provenance of AI-generated vs user-corrected metadata.

### FR-7: Host app integration
- System should be designed so a host app integration layer can invoke analysis on selected photos.
- Likely first host: Lightroom Classic.
- Integration should support sending selected photos for analysis and writing results back as metadata, labels, flags, or ratings.

### FR-8: Search and filtering foundation
- Metadata should be structured to support later search use cases such as:
  - species filtering
  - in-flight vs perched
  - keeper ranking
  - best-of-burst selection
  - date/location-aware species reranking

---

## Non-functional requirements

### NFR-1: Performance
- The system should be reasonably fast on large batches.
- The design should support use of embedded JPEG previews or reduced-resolution preview paths for early-stage detection to improve speed.

### NFR-2: Accuracy
- Bird detection and focus scoring must be useful enough to save time, even if imperfect.
- False negatives on strong keeper images should be minimized.
- Confidence outputs should be exposed rather than hidden.

### NFR-3: Extensibility
- The system must allow additional AI metadata tasks later without redesigning the core pipeline.
- New models should be addable as independent pipeline stages.

### NFR-4: Interoperability
- The system should preserve compatibility with existing photo workflows and metadata standards where possible.
- Standard metadata fields should be used when appropriate; custom metadata should be namespaced and documented.

### NFR-5: Auditability
- Outputs should be traceable to model versions and analysis runs.
- Metadata should include enough provenance to debug bad predictions.

---

## 6. Recommended Product Framing

Frame the project as:

**AI-assisted bird photo metadata engine with culling as the first workflow**

This framing is better than a narrow "bird focus checker" because it supports near-term and long-term capabilities with the same architecture.

---

## 7. High-Level Software Components

## Component A: Ingestion Layer
Purpose:
- Discover images to analyze
- Read file paths and source metadata
- Extract embedded previews or generate analysis previews
- Collect EXIF and related deterministic metadata

Possible responsibilities:
- file enumeration
- RAW format handling
- EXIF extraction
- preview extraction
- batching / job creation

Inputs:
- RAW files
- folders
- selected image lists from host app

Outputs:
- normalized image job records
- preview images for model inference
- deterministic metadata payloads

## Component B: Analysis Orchestrator
Purpose:
- Manage the analysis pipeline for each image or batch
- Coordinate calls to detection, classification, focus, and metadata modules
- Aggregate outputs into one per-image result object

Possible responsibilities:
- task scheduling
- batch orchestration
- retry/failure handling
- pipeline configuration
- model version tracking

Inputs:
- normalized image job records
- preview images
- deterministic metadata
- model config

Outputs:
- consolidated inference result per image
- batch-level manifest
- provenance metadata

## Component C: Bird Detection Module
Purpose:
- Determine whether birds are present
- Localize birds in the image

Possible responsibilities:
- object detection
- candidate box ranking
- primary-subject selection
- small-subject thresholding

Outputs:
- bird_present
- bird_count
- bird_boxes
- detection_confidence
- primary_subject_box

## Component D: Focus / Quality Scoring Module
Purpose:
- Estimate whether the primary bird subject is acceptably sharp and usable

Possible responsibilities:
- crop-based sharpness analysis
- head/eye weighting if available
- motion blur estimation
- keeper score generation

Outputs:
- focus_score
- head_focus_score
- motion_blur_score
- obstruction_score
- eye_visible
- keeper_score

## Component E: Species and Context Inference Module
Purpose:
- Infer probable species and related semantic tags

Possible responsibilities:
- species top-k classification
- date/location-aware reranking
- action/context tagging
- habitat/background cue extraction

Outputs:
- primary_species_guess
- species_top_k
- species_confidences
- context_tags
- action_tags

## Component F: Metadata Layer
Purpose:
- Convert raw inference outputs into usable metadata for users and downstream tools

Possible responsibilities:
- map outputs to standard metadata fields
- map outputs to custom metadata fields
- generate sidecars / manifests
- preserve model provenance

Outputs:
- XMP sidecars or updates
- JSON metadata manifests
- catalog-ready metadata payloads

## Component G: Host Application Integration Layer
Purpose:
- Connect the standalone analysis engine to photo workflow software

Likely first target:
- Lightroom Classic

Possible responsibilities:
- invoke analysis for selected photos
- poll for job completion
- import results
- write flags, labels, ratings, or custom metadata

Outputs:
- user-visible metadata in host app
- workflow actions based on model outputs

## Component H: Review UI / Feedback Loop
Purpose:
- Let the user inspect results and correct mistakes
- Capture corrected labels for later training/evaluation

Possible responsibilities:
- show bird crop and scores
- approve/reject species guess
- override focus result
- mark best image in burst
- export corrected training labels

Outputs:
- corrected metadata
- labeled training/evaluation data
- QA and model improvement signals

---

## 8. Basic Data Flow

## End-to-end flow

1. User selects a folder of RAW files or selects images from a host app.
2. Ingestion layer discovers files and extracts deterministic metadata.
3. Ingestion layer obtains an analysis image:
   - preferably embedded preview for fast first-pass inference
   - optionally higher-resolution crop path for detailed focus scoring
4. Analysis orchestrator creates per-image analysis jobs.
5. Bird detection module determines:
   - whether a bird is present
   - how many birds are present
   - where the birds are located
6. If a bird is detected, the system selects a primary subject box.
7. Focus module evaluates the bird crop and outputs sharpness/keeper-related scores.
8. Species/context module evaluates the bird crop and outputs:
   - species candidates
   - confidence scores
   - semantic/context tags
9. Metadata layer merges:
   - deterministic metadata
   - AI metadata
   - provenance metadata
10. Results are persisted to sidecar/catalog/manifests.
11. Host app integration layer imports results into the workflow environment.
12. User reviews and corrects outputs.
13. Corrections are captured for future model evaluation and retraining.

---

## 9. Suggested Metadata Schema (High Level)

## Deterministic metadata
- file_path
- file_name
- capture_datetime
- camera_model
- lens_model
- focal_length
- aperture
- shutter_speed
- iso
- gps_lat
- gps_lon
- burst_sequence_id (if derivable)
- image_dimensions

## AI metadata
- bird_present
- bird_count
- bird_boxes
- primary_subject_box
- detection_confidence
- focus_score
- head_focus_score
- eye_visible
- motion_blur_score
- obstruction_score
- subject_size_percent
- keeper_score
- primary_species_guess
- species_top_k
- species_confidences
- action_tags
- context_tags

## Provenance metadata
- analysis_timestamp
- pipeline_version
- detector_model_name
- detector_model_version
- classifier_model_name
- classifier_model_version
- focus_model_name
- focus_model_version
- reranking_inputs_used
- user_override_flags

---

## 10. Recommended Technical Approach

## Architecture recommendation
Use a standalone analysis engine plus thin host-app integration.

Why:
- AI and computer vision logic will likely require Python and modern ML tooling.
- Host application plugin environments are usually too limited for the core analysis logic.
- A thin integration layer is easier to maintain.

## Likely implementation shape
- desktop or local service for analysis engine
- plugin or script layer for Lightroom Classic
- metadata sidecars and/or manifest files for interchange
- optional local database for job and result tracking

## Suggested implementation phases
- Phase 1: standalone batch analyzer
- Phase 2: metadata export and review tooling
- Phase 3: Lightroom integration
- Phase 4: feedback-driven retraining and search UX
- Phase 5: advanced birding workflows such as checklist-aware reranking or burst summarization

---

## 11. AI Opportunities Beyond MVP

Once bird detection and focus scoring work, the same pipeline can be extended to:

- species prediction
- burst deduplication / best-frame ranking
- perched vs in-flight classification
- waterbird / shorebird / raptor / songbird broad categories
- behavior tags such as feeding, perched, flying, wading
- date/location-aware species reranking
- semantic search
- eBird checklist correlation
- habitat tagging
- rarity flagging with explicit caution and low automation

---

## 12. Risks and Design Cautions

## Product risks
- "In focus" is subjective and bird-specific; generic sharpness metrics may not be useful.
- Species classification may be overtrusted by users if confidence is not clearly surfaced.
- Bird detection may over-trigger on statues, signs, or very small distant shapes.
- Small birds, occlusion, motion blur, and cluttered habitats will be challenging.

## Technical risks
- RAW decoding and preview extraction can become a bottleneck.
- Metadata writing must avoid damaging or confusing the user’s existing workflow.
- Plugin integration may be limited by host app SDK constraints.
- Model inference performance may be slow on large datasets without batching or hardware acceleration.

## Data risks
- Training data licensing must be handled carefully.
- User corrections must be separated from unverified AI predictions.
- Scientific correctness should not be overstated.

---

## 13. Open Design Questions for Planning Agent

The planning agent should help resolve:

1. What is the best first host environment:
   - standalone only
   - Lightroom Classic first
   - CLI plus Lightroom plugin
2. What metadata persistence strategy should be primary:
   - XMP sidecars
   - host-app-only metadata
   - JSON manifests plus optional XMP
3. Should v1 focus only on:
   - bird present
   - focus score
   - keeper score
   or also include species top-k
4. Should focus scoring be:
   - mostly classical CV + heuristics first
   - model-based from the beginning
5. What review UX is needed for v1:
   - no UI, just metadata
   - lightweight local review tool
   - host-app-centric correction flow
6. What operating system should be primary for the initial build:
   - macOS
   - Windows
   - cross-platform desktop
7. What hardware targets should be supported:
   - CPU only
   - Apple Silicon
   - NVIDIA CUDA
   - mixed acceleration paths

---

## 14. Recommended MVP Definition

A practical MVP could be defined as:

### Inputs
- folder of RAW bird photos

### Outputs per image
- bird_present
- bird_boxes
- focus_score
- keeper_score
- optional species_top_3

### Persistence
- JSON manifest plus optional XMP sidecar

### User interaction
- analyze batch
- review results
- filter to likely keepers and likely no-bird frames

### Success criteria
- reduces manual culling time
- catches most obvious no-bird frames
- surfaces a large fraction of sharp bird keepers
- produces metadata useful enough to support filtering/search

---

## 15. Recommendation to Planning Agent

Treat this as a metadata-enrichment platform with a bird-photo culling MVP.

Prioritize:
1. architecture and metadata schema
2. standalone analyzer
3. review/correction loop
4. Lightroom integration
5. advanced species/context enrichment

Avoid over-scoping around editing, DAM replacement, or highly granular ornithological claims in v1.

---

## 16. Compact LLM-Parseable Summary

## Project
AI-assisted bird photo metadata engine

## MVP objective
Analyze RAW bird photos and attach metadata that helps with culling.

## MVP core outputs
- bird_present
- bird_boxes
- focus_score
- keeper_score
- optional species_top_k

## Primary architecture
- standalone analysis engine
- metadata output layer
- optional Lightroom Classic integration layer

## Core modules
- ingestion
- analysis orchestrator
- bird detector
- focus scorer
- species/context inference
- metadata writer
- host app integration
- review/feedback UI

## Key design principle
Treat AI as a metadata enrichment pipeline, not just a keep/reject classifier.

## Main user value
Faster culling, better search, richer wildlife-photo organization.

## Main planning priorities
- define metadata schema
- choose persistence strategy
- define MVP boundaries
- decide host integration strategy
- design correction feedback loop
