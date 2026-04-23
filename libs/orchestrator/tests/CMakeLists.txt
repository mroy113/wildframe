add_executable(wildframe_orchestrator_test
    paths_test.cpp
)
wildframe_apply_tests_tidy_config(wildframe_orchestrator_test)

target_link_libraries(wildframe_orchestrator_test
    PRIVATE
        wildframe::orchestrator
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(wildframe_orchestrator_test)
