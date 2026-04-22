add_library(wildframe_log STATIC
    src/log.cpp
)
add_library(wildframe::log ALIAS wildframe_log)

find_package(spdlog CONFIG REQUIRED)

target_include_directories(wildframe_log
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# spdlog is part of the public surface: the log header includes
# <spdlog/spdlog.h> so the WILDFRAME_LOG_* macros can expand to
# SPDLOG_LOGGER_* invocations. Every module that emits log lines
# inherits the dependency transitively via wildframe::log.
target_link_libraries(wildframe_log
    PUBLIC
        spdlog::spdlog
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
