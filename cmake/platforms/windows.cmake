# Windows platform minimums — STUB.
#
# Windows is explicitly out of scope for MVP (CLAUDE.md §6, handoff §6).
# This file exists so the cmake/PlatformMinimums.cmake dispatcher has a
# landing spot when cross-compile support is scoped after Phase 2 of
# development. Keep it as a stub; do not populate without a plan-change
# PR per CONTRIBUTING.md.
#
# When the time comes:
# - pre_project:  set any MSVC / clang-cl toolchain defaults that must
#                 be visible before project() runs the compiler probes
#                 (e.g. CMAKE_MSVC_RUNTIME_LIBRARY).
# - post_project: require a concrete MSVC (_MSC_VER) or clang-cl version
#                 that supports the C++20 features Wildframe uses.
#                 Mirror the shape of cmake/platforms/macos.cmake.
# - Update docs/DEV_SETUP.md §1 to document the Windows row.

if(WILDFRAME_PLATFORM_PHASE STREQUAL "pre_project")
    # No-op: lets the dispatcher run through to the clearer
    # post_project error below, where the compiler is known.
elseif(WILDFRAME_PLATFORM_PHASE STREQUAL "post_project")
    message(FATAL_ERROR
        "Wildframe: Windows builds are not supported yet "
        "(CLAUDE.md §6 — Windows/Linux out of MVP scope). "
        "Populate cmake/platforms/windows.cmake and update "
        "docs/DEV_SETUP.md when scoping cross-compile support.")
endif()
