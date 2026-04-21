add_library(wildframe_metadata STATIC
    src/metadata.cpp
)
add_library(wildframe::metadata ALIAS wildframe_metadata)

target_include_directories(wildframe_metadata
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
