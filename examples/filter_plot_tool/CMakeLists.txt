cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 17)

add_executable(filter_plot_tool
    filter_plot_tool.cpp
)

# Fectch matplotlib-cpp
message(STATUS "Fetching matplotlib-cpp")
file(DOWNLOAD https://raw.githubusercontent.com/lava/matplotlib-cpp/master/matplotlibcpp.h ${CMAKE_CURRENT_BINARY_DIR}/matplotlib-cpp/matplotlibcpp.h)
target_include_directories(filter_plot_tool PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/matplotlib-cpp)

# link to python
find_package(Python3 COMPONENTS Development)
target_compile_definitions(filter_plot_tool
    PUBLIC
        JUCE_ALLOW_STATIC_NULL_VARIABLES=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1

        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CAMERA=disabled

        # These are purposefully not overridden.
        # JUCE_DISPLAY_SPLASH_SCREEN=0
        # JUCE_REPORT_APP_USAGE=0

        JUCE_COREGRAPHICS_DRAW_ASYNC=1
        JUCE_WASAPI=1
        JUCE_DIRECTSOUND=1

        JUCE_CATCH_UNHANDLED_EXCEPTIONS=0
        JUCE_VST3_CAN_REPLACE_VST2=0

    PRIVATE
        WITHOUT_NUMPY=1
)

target_include_directories(filter_plot_tool PRIVATE ${Python3_INCLUDE_DIRS})
target_link_libraries(filter_plot_tool PRIVATE Python3::Python sst-filters sst-filters-extras simde juce::juce_dsp)

add_custom_command(TARGET filter_plot_tool
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E echo "Copying $<TARGET_FILE:filter_plot_tool> to example-binaries"
        COMMAND ${CMAKE_COMMAND} -E make_directory example-binaries
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:filter_plot_tool>" example-binaries)
