# Platform-minimums dispatcher. Included from the top-level CMakeLists.txt
# twice — once before project() and once after — with
# WILDFRAME_PLATFORM_PHASE set to "pre_project" or "post_project". The
# dispatcher routes to the correct cmake/platforms/<os>.cmake based on
# CMAKE_HOST_* (pre-project, before project() populates CMAKE_SYSTEM_NAME)
# or APPLE / WIN32 / UNIX (post-project, from project()).
#
# MVP targets macOS only (CLAUDE.md §6). Linux and Windows files are
# stubs that land once cross-compile support is scoped (post Phase 2).
#
# Per-platform files receive WILDFRAME_PLATFORM_PHASE and branch on it.

if(NOT DEFINED WILDFRAME_PLATFORM_PHASE)
    message(FATAL_ERROR
        "PlatformMinimums: WILDFRAME_PLATFORM_PHASE must be set to "
        "\"pre_project\" or \"post_project\" before include().")
endif()

set(_wildframe_platforms_dir "${CMAKE_CURRENT_LIST_DIR}/platforms")

if(WILDFRAME_PLATFORM_PHASE STREQUAL "pre_project")
    if(CMAKE_HOST_APPLE)
        include("${_wildframe_platforms_dir}/macos.cmake")
    elseif(CMAKE_HOST_WIN32)
        include("${_wildframe_platforms_dir}/windows.cmake")
    elseif(CMAKE_HOST_UNIX)
        include("${_wildframe_platforms_dir}/linux.cmake")
    else()
        message(FATAL_ERROR
            "PlatformMinimums: unrecognized host platform. "
            "See docs/DEV_SETUP.md.")
    endif()
elseif(WILDFRAME_PLATFORM_PHASE STREQUAL "post_project")
    if(APPLE)
        include("${_wildframe_platforms_dir}/macos.cmake")
    elseif(WIN32)
        include("${_wildframe_platforms_dir}/windows.cmake")
    elseif(UNIX)
        include("${_wildframe_platforms_dir}/linux.cmake")
    else()
        message(FATAL_ERROR
            "PlatformMinimums: unrecognized target platform "
            "(CMAKE_SYSTEM_NAME='${CMAKE_SYSTEM_NAME}'). "
            "See docs/DEV_SETUP.md.")
    endif()
else()
    message(FATAL_ERROR
        "PlatformMinimums: WILDFRAME_PLATFORM_PHASE must be "
        "\"pre_project\" or \"post_project\", got "
        "\"${WILDFRAME_PLATFORM_PHASE}\".")
endif()

unset(_wildframe_platforms_dir)
