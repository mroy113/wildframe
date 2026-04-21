add_library(wildframe_orchestrator STATIC
    src/orchestrator.cpp
)
add_library(wildframe::orchestrator ALIAS wildframe_orchestrator)

target_include_directories(wildframe_orchestrator
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(wildframe_orchestrator PUBLIC cxx_std_20)
