add_executable(wildframe_focus_test
    focus_test.cpp
    focus_stage_test.cpp
)

target_link_libraries(wildframe_focus_test
    PRIVATE
        wildframe::focus
        wildframe::detect
        wildframe::raw
        wildframe::orchestrator
        wildframe::log
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(wildframe_focus_test)
