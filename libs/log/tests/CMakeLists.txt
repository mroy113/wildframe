add_executable(wildframe_log_test
    log_test.cpp
)
wildframe_apply_tests_tidy_config(wildframe_log_test)

target_link_libraries(wildframe_log_test
    PRIVATE
        wildframe::log
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(wildframe_log_test)
