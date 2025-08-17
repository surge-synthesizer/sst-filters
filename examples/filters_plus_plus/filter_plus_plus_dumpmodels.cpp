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

#include "sst/filters++.h"

int main(int, char **)
{
    namespace sfpp = sst::filtersplusplus;

    auto models = sfpp::Filter::availableModels();

    for (auto &mod : models)
    {
        std::cout << "\n";
        std::cout << "# Model: `" << sfpp::toString(mod) << "` (0x" << std::hex << (int)mod
                  << std::dec << ")" << std::endl;
        auto subm = sfpp::Filter::availableModelConfigurations(mod, true);
        for (auto s : subm)
        {
            if (s == sfpp::ModelConfig())
            {
                std::cout << "   - No Submodels" << std::endl;
                continue;
            }
            auto [pt, st, dt, smt] = s;
            std::cout << "   -";
            std::string pfx = " ";
            if (pt != sfpp::PassTypes::UNSUPPORTED)
            {
                std::cout << pfx << "PassType=`" << sfpp::toString(pt) << "` (0x" << std::hex
                          << (int)pt << ")";
                pfx = "; ";
            }
            if (st != sfpp::SlopeLevels::UNSUPPORTED)
            {
                std::cout << pfx << "SlopeLevel=`" << sfpp::toString(st) << "` (0x" << std::hex
                          << (int)st << ")";
                pfx = "; ";
            }
            if (dt != sfpp::DriveTypes::UNSUPPORTED)
            {
                std::cout << pfx << "DriveType=`" << sfpp::toString(dt) << "` (0x" << std::hex
                          << (int)dt << ")";
                pfx = "; ";
            }
            if (smt != sfpp::SubModelTypes::UNSUPPORTED)
            {
                std::cout << pfx << "SubModelType=`" << sfpp::toString(smt) << "` (0x" << std::hex
                          << (int)smt << ")";
                pfx = "; ";
            }

            std::cout << std::endl;
        }
    }
}