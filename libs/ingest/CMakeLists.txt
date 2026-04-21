add_library(wildframe_ingest STATIC
    src/ingest.cpp
)
add_library(wildframe::ingest ALIAS wildframe_ingest)

target_include_directories(wildframe_ingest
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(wildframe_ingest PUBLIC cxx_std_20)
