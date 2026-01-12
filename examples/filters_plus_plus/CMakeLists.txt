cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)

set(FPEXE filter_plus_plus_bruteforce)
add_executable(${FPEXE}
    filter_plus_plus_bruteforce.cpp
)
set(FPDUMP filter_plus_plus_dumpmodels)
add_executable(${FPDUMP}
        filter_plus_plus_dumpmodels.cpp
)


if (APPLE)
    target_compile_options(${FPEXE} PRIVATE -Wno-deprecated-declarations -Werror)
    target_compile_options(${FPDUMP} PRIVATE -Wno-deprecated-declarations -Werror)
endif()

# Fectch matplotlib-cpp
message(STATUS "Fetching stb_image_write")
file(DOWNLOAD https://raw.githubusercontent.com/libigl/libigl-stb/refs/heads/master/stb_image_write.h ${CMAKE_CURRENT_BINARY_DIR}/stb/stb_image_write.h)
target_include_directories(${FPEXE} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/stb)
target_link_libraries(${FPEXE} PRIVATE sst-filters sst-basic-blocks simde)
target_link_libraries(${FPDUMP} PRIVATE sst-filters sst-basic-blocks simde)

if (APPLE)
    target_compile_definitions(${FPEXE} PRIVATE MAC_FILE)
elseif (WIN32)
    target_compile_definitions(${FPEXE} PRIVATE WIN_FILE)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU" AND (NOT (CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "MSVC")))
    if ("${CMAKE_CXX_STANDARD}" GREATER_EQUAL 20)
        message(STATUS "clap-wrapper: Turning off char8_t c++20 changes")
        target_compile_options(${FPEXE} PRIVATE -fno-char8_t)
        target_compile_options(${FPDUMP} PRIVATE -fno-char8_t)
    endif()
endif()
# others are the fallthrough. this is just for files
