message(STATUS "Configuring sst-filters examples...")

# The examples need JUCE
CPMAddPackage("gh:juce-framework/JUCE#6.1.5")

add_subdirectory(filter_plot_tool)
add_subdirectory(filters_example_plugin)
