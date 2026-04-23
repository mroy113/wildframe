# Wildframe CLI / GUI entry point (Module 7). TB-01 lands the CLI
# surface (arg parser, TOML config loader, log init). TB-02 will
# extend `Run()` to construct and invoke the orchestrator, and
# Module 7 later adds the Qt GUI alongside this CLI per handoff §10.
#
# Parser code is split out into a small static library so unit tests
# link against it without pulling in `main.cpp`.

find_package(tomlplusplus CONFIG REQUIRED)

add_library(wildframe_cli STATIC
    cli_args.cpp
    config.cpp
    run.cpp
)
add_library(wildframe::cli ALIAS wildframe_cli)

target_include_directories(wildframe_cli
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# `WILDFRAME_VERSION` is injected so the --version string tracks the
# `project()` declaration in the top-level `CMakeLists.txt` without a
# second place to update.
target_compile_definitions(wildframe_cli
    PRIVATE
        WILDFRAME_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(wildframe_cli
    PUBLIC
        wildframe::log
        wildframe::orchestrator
    PRIVATE
        tomlplusplus::tomlplusplus
)

add_executable(wildframe
    main.cpp
)

# Module links inherited from TB-02's wiring: TB-02 will drop the
# direct links to ingest/raw/detect/focus/metadata here once the
# orchestrator target fans them out transitively (M7-11 cleanup). For
# TB-01 they are retained verbatim so the existing link graph does
# not change.
target_link_libraries(wildframe
    PRIVATE
        wildframe::cli
        wildframe::ingest
        wildframe::raw
        wildframe::detect
        wildframe::focus
        wildframe::metadata
        wildframe::orchestrator
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
