# Linux platform minimums — STUB.
#
# Linux is explicitly out of scope for MVP (CLAUDE.md §6, handoff §6).
# This file exists so the cmake/PlatformMinimums.cmake dispatcher has a
# landing spot when cross-compile support is scoped after Phase 2 of
# development. Keep it as a stub; do not populate without a plan-change
# PR per CONTRIBUTING.md.
#
# When the time comes:
# - pre_project:  set any cross-compile sysroot / toolchain defaults
#                 that must be visible before project() runs the
#                 compiler probes.
# - post_project: require a concrete GCC or Clang version that supports
#                 the C++20 features Wildframe uses (libc++ /libstdc++
#                 feature parity with Apple Clang 15). Mirror the shape
#                 of cmake/platforms/macos.cmake.
# - Update docs/DEV_SETUP.md §1 to document the Linux row.

if(WILDFRAME_PLATFORM_PHASE STREQUAL "pre_project")
    # No-op: lets the dispatcher and vcpkg run through to the clearer
    # post_project error below, where the compiler is known.
elseif(WILDFRAME_PLATFORM_PHASE STREQUAL "post_project")
    message(FATAL_ERROR
        "Wildframe: Linux builds are not supported yet "
        "(CLAUDE.md §6 — Windows/Linux out of MVP scope). "
        "Populate cmake/platforms/linux.cmake and update "
        "docs/DEV_SETUP.md when scoping cross-compile support.")
endif()
