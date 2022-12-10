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
endif()

add_library(${PROJECT_NAME}-extras INTERFACE)
target_include_directories(${PROJECT_NAME}-extras INTERFACE include-extras)
target_link_libraries(${PROJECT_NAME}-extras INTERFACE ${PROJECT_NAME})

get_directory_property(parent_dir PARENT_DIRECTORY)
if ("${parent_dir}" STREQUAL "")
    set(is_toplevel 1)
    include(cmake/CPM.cmake)
else ()
    set(is_toplevel 0)
endif ()
option(SST_FILTERS_BUILD_TESTS "Add targets for building and running sst-filters tests" ${is_toplevel})
option(SST_FILTERS_BUILD_EXAMPLES "Add targets for building and running sst-filters examples" OFF)

if (SST_FILTERS_BUILD_TESTS OR SST_FILTERS_BUILD_EXAMPLES)
    message(STATUS "Importing SIMDE with CPM")
    CPMAddPackage(NAME simde
            GITHUB_REPOSITORY simd-everywhere/simde
            VERSION 0.7.2
            )
    add_library(simde INTERFACE)
    target_include_directories(simde INTERFACE ${simde_SOURCE_DIR})
endif ()

if (SST_FILTERS_BUILD_TESTS)
    add_subdirectory(tests)
endif ()

if(SST_FILTERS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
