cmake_minimum_required(VERSION 3.24)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Platform-minimums dispatcher, pre-project phase. Sets things that
# must be visible to the compiler/feature probes project() triggers
# (e.g. CMAKE_OSX_DEPLOYMENT_TARGET on macOS). See
# cmake/PlatformMinimums.cmake and cmake/platforms/<os>.cmake.
set(WILDFRAME_PLATFORM_PHASE "pre_project")
include(PlatformMinimums)

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

# Platform-minimums dispatcher, post-project phase. Runs compiler /
# toolchain version gates now that CMAKE_CXX_COMPILER_ID and
# CMAKE_CXX_COMPILER_VERSION are populated.
set(WILDFRAME_PLATFORM_PHASE "post_project")
include(PlatformMinimums)
unset(WILDFRAME_PLATFORM_PHASE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Always export compile_commands.json so clang-tidy and clangd work
# out of the box (NFR-8).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# clang-tidy / clang-format wiring and the `format`/`format-check` targets.
include(ClangTooling)

# Configure-time fetch of pinned ONNX model weights (S0-11).
include("${CMAKE_SOURCE_DIR}/tools/fetch_models.cmake")

# Configure-time fetch of pinned CR3 test fixtures (S0-12).
include("${CMAKE_SOURCE_DIR}/tools/fetch_fixtures.cmake")

# enable_testing() must precede any add_subdirectory() that registers
# tests via add_test() / gtest_discover_tests(). BUILD_TESTING defaults
# to ON; building with -DBUILD_TESTING=OFF skips the test executables
# (e.g. for a release packaging configure that does not need them).
include(CTest)
if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
endif()

# Internal static-library modules (see handoff doc §10). wildframe_log
# is listed first because the policy in docs/STYLE.md §4 has every
# other module call into it for log emission once they grow real code.
add_subdirectory(libs/log)
add_subdirectory(libs/ingest)
add_subdirectory(libs/raw)
add_subdirectory(libs/detect)
add_subdirectory(libs/focus)
add_subdirectory(libs/metadata)
add_subdirectory(libs/orchestrator)

# GUI executable target.
add_subdirectory(src)
