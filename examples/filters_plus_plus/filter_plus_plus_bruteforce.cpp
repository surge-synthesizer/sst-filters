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

// I just use STB here to make quick and dirty plots. sst-filters doesn't need it.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <vector>

#include "sst/filters++.h"

struct PNGPlot
{
    static constexpr size_t w{500}, h{500}, ch{3};
    unsigned char data[w * h * ch];
    PNGPlot() { memset(data, 0xCC, sizeof(data)); }

    void save(const char *fn) { stbi_write_png(fn, w, h, ch, data, w * ch); }

    using curve_t = std::vector<std::pair<float, float>>;
    void add(const curve_t &points, int r = 0, int g = 0, int b = 0)
    {
        assert(!points.empty());
        for (auto &n : points)
        {
            auto nf = toPixels(n);
            auto x = nf.first;
            auto y = nf.second;

            for (int dx = -1; dx <= 1; ++dx)
            {
                for (int dy = -1; dy <= 1; ++dy)
                {
                    data[(x + dx + (y + dy) * h) * ch] = r;
                    data[(x + dx + (y + dy) * h) * ch + 1] = g;
                    data[(x + dx + (y + dy) * h) * ch + 2] = b;
                }
            }
        }
    }

    std::pair<int, int> toPixels(const std::pair<float, float> &p)
    {
        auto xp = std::clamp((p.first - xMin) / (xMax - xMin) * (w - 8), 0.f, w - 8.f);
        auto yp = std::clamp(h - 8 - (p.second - yMin) / (yMax - yMin) * (h - 8), 0.f, h - 8.f);
        return {xp + 4, yp + 4};
    }

    float xMin = 0, xMax = 127, yMin = -30, yMax = 12;
};

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

static float
    buffer[4][sst::filters::utilities::MAX_FB_COMB + sst::filters::utilities::SincTable::FIRipol_M];

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
                    if (!filter.prepareInstance())
                        std::cout << "SETUP ERROR" << std::endl;
                },
                [c, res](auto &filter, int voice) { filter.makeCoefficients(voice, c, res); });
            std::cout << "   co=" << c << " res=" << res << std::endl;

            auto [r, g, b] = colors[ci];
            ci = (ci + 1) % colors.size();
            plot.add(lpRes, r, g, b);
        }
    }

    plot.save("/tmp/plot.png");
    system("open /tmp/plot.png");
#endif
}