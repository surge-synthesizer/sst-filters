cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 17)

set(FPEXE filter_plus_plus_bruteforce)
add_executable(${FPEXE}
    filter_plus_plus_bruteforce.cpp
)

if (APPLE)
    target_compile_options(${FPEXE} PRIVATE -Wno-deprecated-declarations -Werror)
endif()

# Fectch matplotlib-cpp
message(STATUS "Fetching stb_image_write")
file(DOWNLOAD https://raw.githubusercontent.com/libigl/libigl-stb/refs/heads/master/stb_image_write.h ${CMAKE_CURRENT_BINARY_DIR}/stb/stb_image_write.h)
target_include_directories(${FPEXE} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/stb)
target_link_libraries(${FPEXE} PRIVATE sst-filters sst-basic-blocks simde)

if (APPLE)
    target_compile_definitions(${FPEXE} PRIVATE MAC_FILE)
elseif (WIN32)
    target_compile_definitions(${FPEXE} PRIVATE WIN_FILE)
endif()
# others are the fallthrough. this is just for files
