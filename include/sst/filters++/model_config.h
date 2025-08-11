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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_MODEL_CONFIG_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_MODEL_CONFIG_H

#include "enums.h"
#include <sstream>

namespace sst::filtersplusplus
{

struct ModelConfig
{
    ModelConfig() {}
    ModelConfig(PassTypes p, SlopeLevels s, DriveTypes d, SubModelTypes m)
        : pt(p), st(s), dt(d), mt(m){};
    ModelConfig(PassTypes p) : pt(p){};
    ModelConfig(SlopeLevels s) : st(s){};
    ModelConfig(PassTypes p, SlopeLevels s, DriveTypes d) : pt(p), st(s), dt(d) {}
    ModelConfig(PassTypes p, DriveTypes d) : pt(p), dt(d) {}
    ModelConfig(PassTypes p, SlopeLevels s) : pt(p), st(s) {}
    ModelConfig(PassTypes p, SubModelTypes m) : pt(p), mt(m) {}
    ModelConfig(PassTypes p, SlopeLevels s, SubModelTypes m) : pt(p), st(s), mt(m) {}
    ModelConfig(SlopeLevels s, SubModelTypes m) : st(s), mt(m) {}

    PassTypes pt{PassTypes::UNSUPPORTED};
    SlopeLevels st{SlopeLevels::UNSUPPORTED};
    DriveTypes dt{DriveTypes::UNSUPPORTED};
    SubModelTypes mt{SubModelTypes::UNSUPPORTED};

    std::string toString() const
    {
        std::ostringstream oss;
        std::string pfx{};
        if (pt != PassTypes::UNSUPPORTED)
        {
            oss << pfx << filtersplusplus::toString(pt);
            pfx = " ";
        }
        if (st != SlopeLevels::UNSUPPORTED)
        {
            oss << pfx << filtersplusplus::toString(st);
            pfx = " ";
        }
        if (dt != DriveTypes::UNSUPPORTED)
        {
            oss << pfx << filtersplusplus::toString(dt);
            pfx = " ";
        }
        if (mt != SubModelTypes::UNSUPPORTED)
        {
            oss << pfx << filtersplusplus::toString(mt);
            pfx = " ";
        }
        return oss.str();
    }
};

inline bool operator==(const ModelConfig &lhs, const ModelConfig &rhs) noexcept
{
    return lhs.pt == rhs.pt && lhs.st == rhs.st && lhs.dt == rhs.dt && lhs.mt == rhs.mt;
}

inline bool operator<(const ModelConfig &lhs, const ModelConfig &rhs) noexcept
{
#define L(x)                                                                                       \
    if (lhs.x != rhs.x)                                                                            \
        return (int)lhs.x < (int)rhs.x;
    L(pt);
    L(st);
    L(dt);
    L(mt);
#undef L
    return false;
}
} // namespace sst::filtersplusplus

template <> struct std::hash<sst::filtersplusplus::ModelConfig>
{
    std::size_t operator()(const sst::filtersplusplus::ModelConfig &s) const noexcept
    {
        auto v1 = (uint32_t)s.pt;
        auto v2 = (uint32_t)s.st;
        auto v3 = (uint32_t)s.dt;
        auto v4 = (uint32_t)s.mt;
        return v1 | (v2 << 7) | (v3 << 18) | (v4 << 25);
        ;
    }
};
#endif // SUPPORT_KEY_H
