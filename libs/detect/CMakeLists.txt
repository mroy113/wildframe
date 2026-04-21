add_library(wildframe_detect STATIC
    src/detect.cpp
)
add_library(wildframe::detect ALIAS wildframe_detect)

target_include_directories(wildframe_detect
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
