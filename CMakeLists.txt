cmake_minimum_required(VERSION 3.24)

# macOS minimum deployment target: 13.0 (Ventura).
# Per handoff §16.24 and docs/DEV_SETUP.md. Must be set before project()
# so the compiler and feature-detection probes see it. The empty-value
# check also covers older caches where the variable is defined but blank.
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING
        "Minimum macOS version Wildframe builds will run on." FORCE)
endif()

# vcpkg toolchain: pick it up from VCPKG_ROOT if the caller did not pass
# -DCMAKE_TOOLCHAIN_FILE explicitly. Must happen before project().
# Presets (S0-04) will wire this explicitly; this hook keeps a bare
# `cmake -S . -B build` working when the env var is set.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "vcpkg toolchain file")
endif()

project(wildframe
    VERSION 0.0.0
    DESCRIPTION "Bird photo culling assistant"
    LANGUAGES CXX
)

# Toolchain minimum: Apple Clang >= 15 (ships with Xcode 15+).
# Per handoff §16.24 and docs/DEV_SETUP.md.
if(APPLE)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        message(FATAL_ERROR
            "Wildframe requires the Apple Clang toolchain on macOS. "
            "Detected: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}. "
            "See docs/DEV_SETUP.md.")
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 15)
        message(FATAL_ERROR
            "Wildframe requires Apple Clang >= 15 (Xcode 15 or newer). "
            "Detected: AppleClang ${CMAKE_CXX_COMPILER_VERSION}. "
            "See docs/DEV_SETUP.md.")
    endif()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Always export compile_commands.json so clang-tidy and clangd work
# out of the box (NFR-8).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# clang-tidy / clang-format wiring and the `format`/`format-check` targets.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(ClangTooling)

# Configure-time fetch of pinned ONNX model weights (S0-11).
include("${CMAKE_SOURCE_DIR}/tools/fetch_models.cmake")

# Configure-time fetch of pinned CR3 test fixtures (S0-12).
include("${CMAKE_SOURCE_DIR}/tools/fetch_fixtures.cmake")

# Internal static-library modules (see handoff doc §10).
add_subdirectory(libs/ingest)
add_subdirectory(libs/raw)
add_subdirectory(libs/detect)
add_subdirectory(libs/focus)
add_subdirectory(libs/metadata)
add_subdirectory(libs/orchestrator)

# GUI executable target.
add_subdirectory(src)
