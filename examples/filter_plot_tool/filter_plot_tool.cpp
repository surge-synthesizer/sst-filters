#include <matplotlibcpp.h>
#include <sst/filters.h>
#include <sst/filters/FilterPlotter.h>
#include <juce_dsp/juce_dsp.h>

namespace plt = matplotlibcpp;

void showHelp()
{
    std::cout << "SST Filter Plotting Tool:" << std::endl;
    std::cout << "Usage: filter_plot_tool <pitch> <res> <filter_type> <filter_subtype>"
              << std::endl;
    std::cout << "    Note that pitch is in units of MIDI note!" << std::endl;
}

int main(int argc, char *argv[])
{
    // parse arguments
    if (argc > 5)
    {
        showHelp();
        return 1;
    }

    if (argc == 2 && strcmp(argv[1], "--help") == 0)
    {
        showHelp();
        return 0;
    }

    float pitch = 0.0f;
    if (argc > 1)
        pitch = (float)std::atof(argv[1]);

    float res = 0.5f;
    if (argc > 2)
        res = (float)std::atof(argv[2]);

    auto filterType = sst::filters::FilterType::fut_lp24;
    if (argc > 3)
        filterType = static_cast<sst::filters::FilterType>(std::atoi(argv[3]));

    auto filterSubType = sst::filters::FilterSubType::st_Standard;
    if (argc > 4)
        filterSubType = static_cast<sst::filters::FilterSubType>(std::atoi(argv[4]));

    // get filter plot data
    sst::filters::FilterPlotter plotter;
    auto [freqAxis, magResponseDBSmoothed] =
        plotter.plotFilterMagnitudeResponse(filterType, filterSubType, pitch, res);

    // make plot
    plt::semilogx(freqAxis, magResponseDBSmoothed);

    plt::title("Filter Magnitude Response");
    plt::grid(true);

    auto maxDBVal = *std::max_element(magResponseDBSmoothed.begin(), magResponseDBSmoothed.end());
    plt::ylim(-60.0f, maxDBVal + 5.0f);
    plt::xlim(20.0f, 20000.0f);

    plt::show();

    return 0;
}
