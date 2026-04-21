add_executable(wildframe
    main.cpp
)

target_link_libraries(wildframe
    PRIVATE
        wildframe::ingest
        wildframe::raw
        wildframe::detect
        wildframe::focus
        wildframe::metadata
        wildframe::orchestrator
)
