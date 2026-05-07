message(STATUS "Add sst-filters-tests")
add_executable(sst-filters-tests)
target_include_directories(sst-filters-tests PRIVATE . ../libs)
target_link_libraries(sst-filters-tests PRIVATE ${PROJECT_NAME} simde)
#target_compile_definitions(sst-filters-tests PRIVATE CATCH_CONFIG_DISABLE_EXCEPTIONS=1)
#target_compile_options(sst-filters-tests PRIVATE -fsanitize=address -fsanitize=undefined)
#target_link_options(sst-filters-tests PRIVATE -fsanitize=address -fsanitize=undefined)
target_sources(sst-filters-tests
        PRIVATE
        BasicFiltersTest.cpp
        BiquadTest.cpp
        ButterworthTest.cpp
        CutoffWarpTest.cpp
        CytomicSVFTests.cpp
        DiodeLadderTest.cpp
        FastTiltNoiseFilterTest.cpp
        K35FilterTest.cpp
        HalfRateTest.cpp
        OBXDFilterTest.cpp
        ResonanceWarpTest.cpp
        TriPoleFilterTest.cpp
        VintageLaddersTest.cpp
        filters_plus_plus.cpp
        tests.cpp
)

# FastTiltNoiseFilter requires SSE4.1 on x86 (alignr_epi8). Probe so the
# flag is silently skipped on architectures where it isn't valid (arm64).
if (NOT MSVC)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-msse4.1" SST_FILTERS_TESTS_SUPPORTS_MSSE41)
    if (SST_FILTERS_TESTS_SUPPORTS_MSSE41)
        target_compile_options(sst-filters-tests PRIVATE -msse4.1)
    endif ()
endif ()

add_custom_command(TARGET sst-filters-tests
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E echo "Copying $<TARGET_FILE:sst-filters-tests> to test-binary"
        COMMAND ${CMAKE_COMMAND} -E make_directory test-binary
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:sst-filters-tests>" test-binary)