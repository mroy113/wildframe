add_library(wildframe_orchestrator STATIC
    src/orchestrator.cpp
    src/paths.cpp
)
add_library(wildframe::orchestrator ALIAS wildframe_orchestrator)

target_include_directories(wildframe_orchestrator
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# `wildframe::ingest` is PUBLIC because `ImageJob` appears in
# `orchestrator.hpp` / `pipeline_stage.hpp` — every consumer of those
# headers needs the ingest include path.
# `wildframe::log` is PRIVATE — no public header re-exports spdlog.
target_link_libraries(wildframe_orchestrator
    PUBLIC
        wildframe::ingest
    PRIVATE
        wildframe::log
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
