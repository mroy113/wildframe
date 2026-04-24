add_library(wildframe_orchestrator STATIC
    src/orchestrator.cpp
    src/paths.cpp
)
add_library(wildframe::orchestrator ALIAS wildframe_orchestrator)

target_include_directories(wildframe_orchestrator
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# `wildframe::ingest`, `wildframe::raw`, and `wildframe::detect` are
# PUBLIC because `ImageJob`, `raw::PreviewImage`, and
# `detect::DetectionResult` all appear in `pipeline_stage.hpp`'s
# `StageContext` — every consumer of that header needs their include
# paths. `raw` landed with TB-03; `detect` landed with TB-04.
# Remaining TB-05..TB-07 thickenings will add `focus` and `metadata`
# under the same rationale.
# `wildframe::log` is PRIVATE — no public header re-exports spdlog.
target_link_libraries(wildframe_orchestrator
    PUBLIC
        wildframe::ingest
        wildframe::raw
        wildframe::detect
    PRIVATE
        wildframe::log
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
