cmake_minimum_required(VERSION 3.24)

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

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Always export compile_commands.json so clang-tidy and clangd work
# out of the box (NFR-8).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Internal static-library modules (see handoff doc §10).
add_subdirectory(libs/ingest)
add_subdirectory(libs/raw)
add_subdirectory(libs/detect)
add_subdirectory(libs/focus)
add_subdirectory(libs/metadata)
add_subdirectory(libs/orchestrator)

# GUI executable target.
add_subdirectory(src)
