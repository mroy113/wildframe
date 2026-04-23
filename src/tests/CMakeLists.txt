add_executable(wildframe_cli_test
    cli_args_test.cpp
    config_test.cpp
)

target_link_libraries(wildframe_cli_test
    PRIVATE
        wildframe::cli
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(wildframe_cli_test)
