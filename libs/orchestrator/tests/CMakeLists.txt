add_executable(wildframe_orchestrator_test
    orchestrator_test.cpp
    paths_test.cpp
)

# `wildframe::log` links directly so the test fixture can `Init()` a
# silent sink — the orchestrator logs through the registry and would
# abort in `detail::native` without a registered tag.
target_link_libraries(wildframe_orchestrator_test
    PRIVATE
        wildframe::orchestrator
        wildframe::log
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(wildframe_orchestrator_test)
