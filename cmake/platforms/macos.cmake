# macOS platform minimums. Per handoff §16.24 and docs/DEV_SETUP.md.
# Dispatched from cmake/PlatformMinimums.cmake; branches on
# WILDFRAME_PLATFORM_PHASE.
#
# Pre-project:  default CMAKE_OSX_DEPLOYMENT_TARGET to 13.0 (Ventura)
#               so compiler and feature-detection probes see it. The
#               empty-value check covers older caches where the
#               variable is defined but blank.
# Post-project: require Apple Clang >= 15 (ships with Xcode 15+).

if(WILDFRAME_PLATFORM_PHASE STREQUAL "pre_project")
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING
            "Minimum macOS version Wildframe builds will run on." FORCE)
    endif()
elseif(WILDFRAME_PLATFORM_PHASE STREQUAL "post_project")
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
