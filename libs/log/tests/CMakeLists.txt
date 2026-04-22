add_executable(wildframe_log_test
    log_test.cpp
)

target_link_libraries(wildframe_log_test
    PRIVATE
        wildframe::log
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(wildframe_log_test)
