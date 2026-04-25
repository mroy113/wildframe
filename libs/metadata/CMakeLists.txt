# `wildframe_metadata` (Module 5). TB-06 thickens the module with the
# public `ReadExif` entry point and `MetadataReadStage`; Exiv2 does not
# link until TB-07 / M5-02 wire the real read and write paths.
add_library(wildframe_metadata STATIC
    src/metadata.cpp
    src/metadata_read_stage.cpp
)
add_library(wildframe::metadata ALIAS wildframe_metadata)

target_include_directories(wildframe_metadata
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# `wildframe::orchestrator` is PRIVATE: `metadata_read_stage.cpp`
# implements `orchestrator::PipelineStage`, but the public
# `metadata_read_stage.hpp` forward-declares the base so metadata
# consumers do not transitively inherit orchestrator's include surface
# (same pattern as `wildframe_raw` / `wildframe_detect` /
# `wildframe_focus`, documented in `docs/ARCHITECTURE.md` §4.5).
# `wildframe::log` is PRIVATE — only used for log emission in the
# stage's `.cpp`.
target_link_libraries(wildframe_metadata
    PRIVATE
        wildframe::log
        wildframe::orchestrator
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
