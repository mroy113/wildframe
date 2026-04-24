# `wildframe_detect` (Module 3). TB-03 introduced `BBox` as a
# header-only type here so `wildframe_raw::DecodeCrop` could reference
# it. TB-04 thickens the module into a STATIC library with the stub
# `Detect` entry point and `DetectStage`; ONNX Runtime still does not
# link until M3-01 wires the real session.
add_library(wildframe_detect STATIC
    src/detect.cpp
    src/detect_stage.cpp
)
add_library(wildframe::detect ALIAS wildframe_detect)

target_include_directories(wildframe_detect
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# `wildframe::raw` is PUBLIC because `raw::PreviewImage` appears in
# the public `Detect` signature (the detector's input).
# `wildframe::orchestrator` is PRIVATE: `detect_stage.cpp` implements
# `orchestrator::PipelineStage`, but the public `detect_stage.hpp`
# forward-declares the base so detect consumers do not transitively
# inherit orchestrator's include surface (same pattern as
# `wildframe_raw`, documented in `docs/ARCHITECTURE.md` §4.5).
# `wildframe::log` is PRIVATE — only used for log emission in the
# stage's `.cpp`.
target_link_libraries(wildframe_detect
    PUBLIC
        wildframe::raw
    PRIVATE
        wildframe::log
        wildframe::orchestrator
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
