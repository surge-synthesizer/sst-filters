/*
 * sst-filters - A header-only collection of SIMD filter
 * implementations by the Surge Synth Team
 *
 * Copyright 2019-2025, various authors, as described in the GitHub
 * transaction log.
 *
 * sst-filters is released under the Gnu General Public Licens
 * version 3 or later. Some of the filters in this package
 * originated in the version of Surge open sourced in 2018.
 *
 * All source in sst-filters available at
 * https://github.com/surge-synthesizer/sst-filters
 */

#include <iostream>
#include <functional>
#include <filesystem>

// I just use STB here to make quick and dirty plots. sst-filters doesn't need it.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "pngplot.h"

static float
    buffer[4][sst::filters::utilities::MAX_FB_COMB + sst::filters::utilities::SincTable::FIRipol_M];

PNGPlot::curve_t
bruteForceResponseCurve(std::function<void(sst::filtersplusplus::Filter &)> config,
                        std::function<void(sst::filtersplusplus::Filter &, int voice)> setCoeff)
{
    int step = 1;
    auto nVoices{4};
    auto nPoints{2500};
    double sr{48000.0}, sri{1.0 / sr};
    size_t blockSize{16};
    PNGPlot::curve_t res;
    for (int i = 0; i < 120; i += nVoices * step)
    {
        auto filter = sst::filtersplusplus::Filter();
        config(filter);

        filter.setSampleRateAndBlockSize(sr, blockSize);
        filter.setQuad();

        if (filter.requiredDelayLinesSizes(filter.getFilterModel(), filter.getModelConfiguration()))
        {
            memset(buffer, 0, sizeof(buffer));
            for (int i = 0; i < 4; ++i)
            {
                filter.provideDelayLine(i, &buffer[i][0]);
            }
        }

        if (!filter.prepareInstance())
        {
            throw std::runtime_error("Failed to prepare instance");
        }

        if (i == 0)
            std::cout << "Processing : " << filter.displayName() << std::endl;

        float fr[nVoices], inf[nVoices], phase[nVoices];
        for (int j = 0; j < nVoices; ++j)
        {
            fr[j] = 440 * pow(2.0, (i - 69 + j * step) / 12.);
            inf[j] = fr[j] * sri;
            phase[0] = 0.f;
        }

        // Now loop
        size_t blockPos = 0;
        double irms[nVoices], orms[nVoices];
        std::fill(irms, irms + nVoices, 0.0);
        std::fill(orms, orms + nVoices, 0.0);

        for (int j = 0; j < nPoints; ++j)
        {
            if (blockPos == 0)
            {
                for (int j = 0; j < nVoices; ++j)
                {
                    setCoeff(filter, j);
                }
                filter.prepareBlock();
            }
            float sinv alignas(16)[nVoices], filtv alignas(16)[nVoices];
            for (int k = 0; k < nVoices; ++k)
            {
                sinv[k] = sinf(2.0 * M_PI * phase[k]);
                irms[k] += sinv[k] * sinv[k];
                phase[k] += inf[k];
                if (phase[k] > 1.f)
                    phase[k] -= 1.f;
            }

            auto in = SIMD_MM(load_ps(sinv));
            SIMD_M128 out = filter.processSample(in);

            SIMD_MM(store_ps(filtv, out));
            for (int k = 0; k < nVoices; ++k)
            {
                orms[k] += filtv[k] * filtv[k];
            }

            blockPos++;
            if (blockPos == blockSize)
            {
                filter.concludeBlock();
                blockPos = 0;
            }
        }

        for (int j = 0; j < nVoices; ++j)
        {
            irms[j] = sqrt(irms[j] / nPoints);
            orms[j] = sqrt(orms[j] / nPoints);
            auto dbCut = 20 * std::log10(orms[j] / irms[j]);
            res.emplace_back(i + j, dbCut);
        }
    }
    return res;
}

int main(int, char **)
{
    namespace sfpp = sst::filtersplusplus;
    PNGPlot plot;

    std::vector<std::array<int, 3>> colors = {
        {255, 0, 0}, {0, 180, 0}, {0, 0, 200}, {255, 0x90, 0}};
    int ci{0};

#define STDIN_PICK 1
#ifdef STDIN_PICK
    std::cout << "Welcome to the quick brute force plotter.\n\n";
    std::cout << "Pick a model\n";
    int idx{0};
    auto modls = sfpp::Filter::availableModels();
    for (auto &m : modls)
    {
        std::cout << idx + 1 << "   - " << sfpp::toString(m) << "\n";
        idx++;
    }
    std::cout << "\nfilt> ";
    std::string inl;
    std::getline(std::cin, inl);
    int idx2 = std::stoi(inl);
    auto model = modls[idx2 - 1];
    std::cout << "\n\nConfigurations of " << sfpp::toString(model) << "\n\n";
    idx = 0;
    auto cf = sfpp::Filter::availableModelConfigurations(model, true);
    for (auto &c : cf)
    {
        std::cout << idx + 1 << "   - " << c.toString() << "\n";
        idx++;
    }
    std::cout << "\nfilt> ";
    std::getline(std::cin, inl);
    idx2 = std::stoi(inl);
    auto cfg = cf[idx2 - 1];

    std::cout << "\n\nMethod\n\n";
    std::cout << "1 - co=440, sweep resonance\n"
              << "2 - sweep co, resonance = 0.3\n";
    std::cout << "\nfilt> ";

    std::getline(std::cin, inl);
    idx2 = std::stoi(inl);

    auto fpath = std::filesystem::temp_directory_path() / "plot.png";
    auto fname = fpath.u8string();

    std::cout << "\n\nOutput File. Press enter for " << fname << std::endl;
    std::cout << "\nfilt> ";
    std::getline(std::cin, inl);
    if (!inl.empty())
    {
        fname = inl;
    }
    std::cout << "\nWriting to " << fname << "\n";

    auto cos = idx2 == 1 ? 0 : -20;
    auto coe = idx2 == 1 ? 0 : 20;
    auto cop = idx2 == 1 ? 1 : 10;

    auto ros = idx2 == 2 ? 0.3 : 0;
    auto roe = idx2 == 2 ? 0.3 : 1;
    auto rop = 0.2;

    ci = 0;
    for (auto c = cos; c <= coe; c += cop)
    {
        for (auto res = ros; res <= roe; res += rop)
        {
            auto lpRes = bruteForceResponseCurve(
                [model, cfg](auto &filter) {
                    filter.setFilterModel(model);
                    filter.setModelConfiguration(cfg);
                },
                [c, res](auto &filter, int voice) { filter.makeCoefficients(voice, c, res); });
            std::cout << "   cutoff = " << c << "\n   res   = " << res << std::endl;

            auto [r, g, b] = colors[ci];
            ci = (ci + 1) % colors.size();
            plot.add(lpRes, r, g, b);
        }
    }

    plot.addTitle(sst::filtersplusplus::toString(model) + " " + cfg.toString());

    plot.save(fname.c_str());
#if MAC_FILE
    fname = "open " + fname;
#endif

#if MAC_FILE || WIN_FILE
    system(fname.c_str());
#else
    std::cout << "File generated to " << fname << std::endl;
#endif
#endif
}