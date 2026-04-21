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

# Resolve CMAKE_CXX_CLANG_TIDY to an absolute path when the `tidy` preset
# (or any caller) has asked for clang-tidy. Apple Command Line Tools does
# not ship clang-tidy, and Homebrew's `llvm` formula is keg-only, so a
# bare `clang-tidy` would not be found in PATH on a default macOS setup.
# Hinting at Homebrew's keg-only locations keeps the `tidy` preset working
# without forcing developers to pollute PATH (which would shadow Apple's
# clang/clang-format/clangd).
if(CMAKE_CXX_CLANG_TIDY)
    find_program(WILDFRAME_CLANG_TIDY_EXECUTABLE
        NAMES clang-tidy
        HINTS
            /opt/homebrew/opt/llvm/bin
            /usr/local/opt/llvm/bin
        DOC "Path to clang-tidy (NFR-8 static-analysis gate)"
        REQUIRED
    )
    set(CMAKE_CXX_CLANG_TIDY "${WILDFRAME_CLANG_TIDY_EXECUTABLE}" CACHE STRING "" FORCE)
endif()

# Internal static-library modules (see handoff doc §10).
add_subdirectory(libs/ingest)
add_subdirectory(libs/raw)
add_subdirectory(libs/detect)
add_subdirectory(libs/focus)
add_subdirectory(libs/metadata)
add_subdirectory(libs/orchestrator)

# GUI executable target.
add_subdirectory(src)
