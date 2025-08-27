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
    ModelConfig(Passband p, Slope s, DriveMode d, FilterSubModel m) : pt(p), st(s), dt(d), mt(m) {}
    ModelConfig(Passband p) : pt(p) {}
    ModelConfig(Slope s) : st(s) {}
    ModelConfig(Passband p, Slope s, DriveMode d) : pt(p), st(s), dt(d) {}
    ModelConfig(Passband p, DriveMode d) : pt(p), dt(d) {}
    ModelConfig(Passband p, Slope s) : pt(p), st(s) {}
    ModelConfig(Passband p, FilterSubModel m) : pt(p), mt(m) {}
    ModelConfig(Passband p, Slope s, FilterSubModel m) : pt(p), st(s), mt(m) {}
    ModelConfig(Slope s, FilterSubModel m) : st(s), mt(m) {}

    Passband pt{Passband::UNSUPPORTED};
    Slope st{Slope::UNSUPPORTED};
    DriveMode dt{DriveMode::UNSUPPORTED};
    FilterSubModel mt{FilterSubModel::UNSUPPORTED};

    std::string toString() const
    {
        std::ostringstream oss;
        std::string pfx{};
        if (pt != Passband::UNSUPPORTED)
        {
            oss << pfx << filtersplusplus::toString(pt);
            pfx = " ";
        }
        if (st != Slope::UNSUPPORTED)
        {
            oss << pfx << filtersplusplus::toString(st);
            pfx = " ";
        }
        if (dt != DriveMode::UNSUPPORTED)
        {
            oss << pfx << filtersplusplus::toString(dt);
            pfx = " ";
        }
        if (mt != FilterSubModel::UNSUPPORTED)
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
