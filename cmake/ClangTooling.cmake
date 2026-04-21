# Clang-based dev-tool integration: clang-tidy (NFR-8 static-analysis gate,
# task S0-04) and clang-format (NFR-7 formatter baseline, task S0-05).
#
# Both tools are shipped only by Homebrew's keg-only `llvm` formula on
# macOS; Apple Command Line Tools does not include them. PATH-based
# lookup would force developers to shadow Apple's clang/clangd, so we
# hint at the keg-only prefixes and resolve to an absolute path.

# ---------------------------------------------------------------------------
# clang-tidy (S0-04). Only resolve when the caller has asked for it
# (e.g. the `tidy` preset sets CMAKE_CXX_CLANG_TIDY=clang-tidy). When set,
# rewrite the variable to an absolute path so the per-target integration
# in downstream `add_library` calls picks it up without further work.
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

# ---------------------------------------------------------------------------
# clang-format (S0-05). `format` applies clang-format in place; `format-check`
# verifies no drift and is the CI gate per NFR-7. Both operate on every C++
# source file under libs/, src/, and tests/. If clang-format is absent, the
# targets are omitted rather than failing configure, so developers without
# Homebrew LLVM installed can still build and run tests.
find_program(WILDFRAME_CLANG_FORMAT_EXECUTABLE
    NAMES clang-format
    HINTS
        /opt/homebrew/opt/llvm/bin
        /usr/local/opt/llvm/bin
    DOC "Path to clang-format (S0-05 style baseline)"
)

if(WILDFRAME_CLANG_FORMAT_EXECUTABLE)
    file(GLOB_RECURSE WILDFRAME_FORMAT_SOURCES
        CONFIGURE_DEPENDS
            "${CMAKE_SOURCE_DIR}/libs/*.cpp"
            "${CMAKE_SOURCE_DIR}/libs/*.hpp"
            "${CMAKE_SOURCE_DIR}/libs/*.h"
            "${CMAKE_SOURCE_DIR}/libs/*.cc"
            "${CMAKE_SOURCE_DIR}/src/*.cpp"
            "${CMAKE_SOURCE_DIR}/src/*.hpp"
            "${CMAKE_SOURCE_DIR}/src/*.h"
            "${CMAKE_SOURCE_DIR}/src/*.cc"
            "${CMAKE_SOURCE_DIR}/tests/*.cpp"
            "${CMAKE_SOURCE_DIR}/tests/*.hpp"
            "${CMAKE_SOURCE_DIR}/tests/*.h"
            "${CMAKE_SOURCE_DIR}/tests/*.cc"
    )

    add_custom_target(format
        COMMAND "${WILDFRAME_CLANG_FORMAT_EXECUTABLE}" -i ${WILDFRAME_FORMAT_SOURCES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Applying clang-format to all in-scope sources"
        VERBATIM
    )

    add_custom_target(format-check
        COMMAND "${WILDFRAME_CLANG_FORMAT_EXECUTABLE}" --dry-run --Werror ${WILDFRAME_FORMAT_SOURCES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Verifying clang-format baseline (CI gate)"
        VERBATIM
    )
else()
    message(STATUS
        "clang-format not found: `format` and `format-check` targets will not be defined. "
        "Install LLVM (e.g. `brew install llvm`) to enable them."
    )
endif()
