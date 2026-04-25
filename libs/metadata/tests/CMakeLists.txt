find_package(exiv2 CONFIG REQUIRED)

add_executable(wildframe_metadata_test
    metadata_test.cpp
    metadata_read_stage_test.cpp
    metadata_write_stage_test.cpp
    sidecar_writer_test.cpp
)

target_link_libraries(wildframe_metadata_test
    PRIVATE
        wildframe::metadata
        wildframe::ingest
        wildframe::orchestrator
        wildframe::log
        Exiv2::exiv2lib
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(wildframe_metadata_test)
