add_executable(wildframe_ingest_test
    enumerate_test.cpp
)
wildframe_apply_tests_tidy_config(wildframe_ingest_test)

target_link_libraries(wildframe_ingest_test
    PRIVATE
        wildframe::ingest
        wildframe::log
        GTest::gtest
        GTest::gtest_main
)

# Pass the S0-12 fixtures directory down so the fixture-backed cases
# can find real CR3 files without hard-coding a build-tree path. The
# variable is set at the top level by tools/fetch_fixtures.cmake.
target_compile_definitions(wildframe_ingest_test
    PRIVATE
        WILDFRAME_FIXTURES_DIR="${WILDFRAME_FIXTURES_DIR}"
)

include(GoogleTest)
gtest_discover_tests(wildframe_ingest_test)
