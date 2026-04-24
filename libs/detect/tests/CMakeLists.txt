add_executable(wildframe_detect_test
    detect_test.cpp
    detect_stage_test.cpp
)

target_link_libraries(wildframe_detect_test
    PRIVATE
        wildframe::detect
        wildframe::raw
        wildframe::orchestrator
        wildframe::log
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(wildframe_detect_test)
