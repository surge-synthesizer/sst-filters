add_executable(sst-filters-tests)
target_include_directories(sst-filters-tests PRIVATE .)
target_link_libraries(sst-filters-tests PRIVATE ${PROJECT_NAME} simde)
target_sources(sst-filters-tests
        PRIVATE
        BasicFiltersTest.cpp
        CutoffWarpTest.cpp
        DiodeLadderTest.cpp
        K35FilterTest.cpp
        HalfRateTest.cpp
        OBXDFilterTest.cpp
        ResonanceWarpTest.cpp
        TriPoleFilterTest.cpp
        VintageLaddersTest.cpp
        tests.cpp
        )

add_custom_command(TARGET sst-filters-tests
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E echo "Copying $<TARGET_FILE:sst-filters-tests> to test-binary"
        COMMAND ${CMAKE_COMMAND} -E make_directory test-binary
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:sst-filters-tests>" test-binary)