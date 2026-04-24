add_library(wildframe_raw STATIC
    src/raw.cpp
    src/raw_stage.cpp
)
add_library(wildframe::raw ALIAS wildframe_raw)

target_include_directories(wildframe_raw
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# `wildframe::detect` is PUBLIC because `BBox` appears in raw.hpp's
# `DecodeCrop` signature (TB-03 landed BBox early so this declaration
# could cite it — see plan PR "TB-03 introduces BBox early").
# `wildframe::orchestrator` is PRIVATE: raw_stage.cpp implements
# `orchestrator::PipelineStage`, but the public `raw_stage.hpp`
# forward-declares it so raw consumers do not transitively inherit
# orchestrator's include surface.
# `wildframe::log` is PRIVATE — used only for log emission in the
# stage's .cpp.
target_link_libraries(wildframe_raw
    PUBLIC
        wildframe::detect
    PRIVATE
        wildframe::log
        wildframe::orchestrator
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
