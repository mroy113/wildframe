#pragma once

/// \file
/// Public umbrella header for `wildframe_ingest` (Module 1).
///
/// Scope per handoff doc §10: directory enumeration, RAW file discovery,
/// and CR3 format validation. Callers that only need a single type can
/// include the per-type header directly; this umbrella is a convenience
/// for the orchestrator and GUI.

#include "wildframe/ingest/enumerate.hpp"
#include "wildframe/ingest/image_job.hpp"
#include "wildframe/ingest/ingest_error.hpp"

namespace wildframe::ingest {}  // namespace wildframe::ingest
