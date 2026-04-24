# `wildframe_focus` (Module 4). TB-05 thickens the module into a
# STATIC library with the public `FocusResult` / `FocusConfig` types,
# the stub `Score` entry point, and `FocusStage`. OpenCV does not link
# until M4-01 wires the real keeper-score computation.
add_library(wildframe_focus STATIC
    src/focus.cpp
    src/focus_stage.cpp
)
add_library(wildframe::focus ALIAS wildframe_focus)

target_include_directories(wildframe_focus
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# `wildframe::raw` is PUBLIC because `raw::PreviewImage` appears in
# the public `Score` signature (the scorer's input). `wildframe::detect`
# is PUBLIC because `detect::BBox` appears in the same signature
# (the primary-subject crop). `wildframe::orchestrator` is PRIVATE:
# `focus_stage.cpp` implements `orchestrator::PipelineStage`, but the
# public `focus_stage.hpp` forward-declares the base so focus
# consumers do not transitively inherit orchestrator's include surface
# (same pattern as `wildframe_raw` / `wildframe_detect`, documented in
# `docs/ARCHITECTURE.md` §4.5). `wildframe::log` is PRIVATE — only
# used for log emission in the stage's `.cpp`.
target_link_libraries(wildframe_focus
    PUBLIC
        wildframe::detect
        wildframe::raw
    PRIVATE
        wildframe::log
        wildframe::orchestrator
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
