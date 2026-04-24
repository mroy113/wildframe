add_library(wildframe_orchestrator STATIC
    src/orchestrator.cpp
    src/paths.cpp
)
add_library(wildframe::orchestrator ALIAS wildframe_orchestrator)

target_include_directories(wildframe_orchestrator
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# `wildframe::ingest`, `wildframe::raw`, `wildframe::detect`, and
# `wildframe::focus` are PUBLIC because `ImageJob`,
# `raw::PreviewImage`, `detect::DetectionResult`, and
# `focus::FocusResult` all appear in `pipeline_stage.hpp`'s
# `StageContext` — every consumer of that header needs their include
# paths. `raw` landed with TB-03; `detect` landed with TB-04;
# `focus` landed with TB-05. TB-06 / TB-07 add `metadata` under the
# same rationale.
# `wildframe::log` is PRIVATE — no public header re-exports spdlog.
target_link_libraries(wildframe_orchestrator
    PUBLIC
        wildframe::ingest
        wildframe::raw
        wildframe::detect
        wildframe::focus
    PRIVATE
        wildframe::log
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
