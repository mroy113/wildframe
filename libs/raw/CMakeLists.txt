add_library(wildframe_raw STATIC
    src/raw.cpp
)
add_library(wildframe::raw ALIAS wildframe_raw)

target_include_directories(wildframe_raw
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(wildframe_raw PUBLIC cxx_std_20)
