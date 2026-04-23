add_library(wildframe_ingest STATIC
    src/enumerate.cpp
)
add_library(wildframe::ingest ALIAS wildframe_ingest)

target_include_directories(wildframe_ingest
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# `wildframe::log` is an implementation detail — we call `log::ingest`
# from enumerate.cpp but no public header re-exports a spdlog symbol.
target_link_libraries(wildframe_ingest
    PRIVATE
        wildframe::log
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
