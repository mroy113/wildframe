add_library(wildframe_focus STATIC
    src/focus.cpp
)
add_library(wildframe::focus ALIAS wildframe_focus)

target_include_directories(wildframe_focus
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(wildframe_focus PUBLIC cxx_std_20)
