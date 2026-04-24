# `wildframe_detect` (Module 3) is header-only in Sprint 2: TB-03
# introduces `BBox` here early (see plan PR "TB-03 introduces BBox
# early") so `wildframe_raw::DecodeCrop` can reference it. No sources
# and no ONNX Runtime link until M3-01.
add_library(wildframe_detect INTERFACE)
add_library(wildframe::detect ALIAS wildframe_detect)

target_include_directories(wildframe_detect
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
