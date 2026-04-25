# `wildframe_metadata` (Module 5). TB-06 thickens the module with the
# public `ReadExif` entry point and `MetadataReadStage`; TB-07 adds the
# provenance-only XMP sidecar writer and its stage, and wires Exiv2 in
# for the first time. M5-02..M5-04 swap the stub read path for a real
# Exiv2-backed reader; M5-05..M5-11 layer the AI namespace on top of
# TB-07's writer.
find_package(exiv2 CONFIG REQUIRED)

add_library(wildframe_metadata STATIC
    src/metadata.cpp
    src/metadata_read_stage.cpp
    src/metadata_write_stage.cpp
    src/sidecar_writer.cpp
)
add_library(wildframe::metadata ALIAS wildframe_metadata)

target_include_directories(wildframe_metadata
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# `wildframe::orchestrator` is PRIVATE: `metadata_read_stage.cpp` and
# `metadata_write_stage.cpp` implement `orchestrator::PipelineStage`,
# but the public stage headers forward-declare the base so metadata
# consumers do not transitively inherit orchestrator's include surface
# (same pattern as `wildframe_raw` / `wildframe_detect` /
# `wildframe_focus`, documented in `docs/ARCHITECTURE.md` §4.5).
# `wildframe::log` is PRIVATE — only used for log emission in the
# stages' `.cpp` files. `Exiv2::exiv2lib` is PRIVATE because the XMP
# sidecar format is the public contract, not Exiv2's types — callers
# see `MetadataError` / `DeterministicMetadata` only (STYLE §3.1,
# ARCHITECTURE §1).
target_link_libraries(wildframe_metadata
    PRIVATE
        wildframe::log
        wildframe::orchestrator
        Exiv2::exiv2lib
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
