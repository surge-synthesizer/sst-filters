cmake_minimum_required(VERSION 3.10)
project(sst-filters VERSION 0.5 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)

if(MSVC)
    target_compile_definitions(${PROJECT_NAME}
        INTERFACE
            _USE_MATH_DEFINES=1 # So that we can have M_PI on MSVC
    )

    target_compile_options(${PROJECT_NAME}
        INTERFACE
            /wd4244 # float to double (@TODO)
            /wd4305 # truncation of variable (@TODO)
    )
endif()

get_directory_property(parent_dir PARENT_DIRECTORY)
if ("${parent_dir}" STREQUAL "")
    set(is_toplevel 1)
else ()
    set(is_toplevel 0)
endif ()
option(SST_FILTERS_BUILD_TESTS "Add targets for building and running sst-filters tests" ${is_toplevel})

if (SST_FILTERS_BUILD_TESTS)
    add_executable(sst-filters-tests)
    target_include_directories(sst-filters-tests PRIVATE tests)
    target_link_libraries(sst-filters-tests PRIVATE ${PROJECT_NAME})
    target_sources(sst-filters-tests
        PRIVATE
            tests/BasicFiltersTest.cpp
            tests/CutoffWarpTest.cpp
            tests/DiodeLadderTest.cpp
            tests/K35FilterTest.cpp
            tests/OBXDFilterTest.cpp
            tests/VintageLaddersTest.cpp
            tests/tests.cpp
    )

    add_custom_command(TARGET sst-filters-tests
            POST_BUILD
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E echo "Copying $<TARGET_FILE:sst-filters-tests> to test-binary"
            COMMAND ${CMAKE_COMMAND} -E make_directory test-binary
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:sst-filters-tests>" test-binary)
endif ()
