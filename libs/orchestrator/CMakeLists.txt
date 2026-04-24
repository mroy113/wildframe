add_library(wildframe_orchestrator STATIC
    src/orchestrator.cpp
    src/paths.cpp
)
add_library(wildframe::orchestrator ALIAS wildframe_orchestrator)

target_include_directories(wildframe_orchestrator
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# `wildframe::ingest` and `wildframe::raw` are PUBLIC because
# `ImageJob` and `raw::PreviewImage` both appear in `pipeline_stage.hpp`'s
# `StageContext` — every consumer of that header needs both include
# paths. `raw` was added with TB-03 when `StageContext` grew the
# `preview` slot; further TB-04..TB-07 thickenings will add
# `detect`, `focus`, and `metadata` under the same rationale.
# `wildframe::log` is PRIVATE — no public header re-exports spdlog.
target_link_libraries(wildframe_orchestrator
    PUBLIC
        wildframe::ingest
        wildframe::raw
    PRIVATE
        wildframe::log
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
