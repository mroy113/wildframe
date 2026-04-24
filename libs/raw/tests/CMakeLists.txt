add_executable(wildframe_raw_test
    raw_test.cpp
    raw_stage_test.cpp
)

target_link_libraries(wildframe_raw_test
    PRIVATE
        wildframe::raw
        wildframe::orchestrator
        wildframe::log
        GTest::gtest
        GTest::gtest_main
)

# Surfaces the S0-12 fixture directory for the path-based smoke case
# (the TB-03 stub does not read the file, but the test still confirms
# real paths flow through unchanged). Defaulted at the top level by
# tools/fetch_fixtures.cmake.
target_compile_definitions(wildframe_raw_test
    PRIVATE
        WILDFRAME_FIXTURES_DIR="${WILDFRAME_FIXTURES_DIR}"
)

include(GoogleTest)
gtest_discover_tests(wildframe_raw_test)
