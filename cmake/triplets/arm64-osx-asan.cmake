# arm64-osx-asan: arm64-osx triplet that propagates ASan + UBSan into every
# vcpkg port build. Used by the `asan` CMake preset only — never by
# `debug`, `release`, or `tidy`.
#
# Why this exists. libc++'s std::vector calls
# __sanitizer_annotate_contiguous_container on every size change so
# AddressSanitizer can paint the unused-capacity tail of the buffer with
# the `fc` shadow byte. ASan's container-overflow check is binary on/off
# at runtime — there is no per-stack-frame suppression. If an
# uninstrumented dependency mutates a libc++ vector through the same
# allocator (e.g. Exiv2::XmpData::add → push_back inside XmpParser::decode),
# the annotations never get updated and the next instrumented destructor
# sweep aborts on the stale shadow.
#
# Bringing every vcpkg port inside the same instrumentation envelope as
# Wildframe code resolves the mismatch at the source: the annotation
# hook always fires, the shadow always tracks reality, and the
# container-overflow check stays on for genuine Wildframe bugs.
#
# See: https://github.com/google/sanitizers/wiki/AddressSanitizerContainerOverflow.
#
# This triplet must stay in lockstep with cmake/platforms/macos.cmake's
# deployment-target gate (S0-20). Bump both together.

set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_DEPLOYMENT_TARGET "13.0")

set(_wf_san_flags "-fsanitize=address,undefined -fno-omit-frame-pointer")
set(VCPKG_C_FLAGS      "${_wf_san_flags}")
set(VCPKG_CXX_FLAGS    "${_wf_san_flags}")
set(VCPKG_LINKER_FLAGS "-fsanitize=address,undefined")
unset(_wf_san_flags)
