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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_DETAILS_CONFIGURATION_SELECTOR_IMPL_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_DETAILS_CONFIGURATION_SELECTOR_IMPL_H

#include "../api.h"

namespace sst::filtersplusplus
{
inline bool isModelConfigValid(const sst::filtersplusplus::FilterModel &fm,
                               const sst::filtersplusplus::ModelConfig &tc)
{
    auto cf = Filter::availableModelConfigurations(fm, true);
    for (auto &c : cf)
        if (c == tc)
            return true;

    return false;
}

inline bool isPartialConfigValid(const sst::filtersplusplus::FilterModel &fm, Passband p)
{
    auto cf = Filter::availableModelConfigurations(fm, true);
    for (auto &c : cf)
    {
        if (get<Passband>(c) == p)
            return true;
    }
    return false;
}

inline bool isPartialConfigValid(const sst::filtersplusplus::FilterModel &fm, Passband p, Slope s)
{
    auto cf = Filter::availableModelConfigurations(fm, true);
    for (auto &c : cf)
    {
        if (get<Passband>(c) == p && get<Slope>(c) == s)
            return true;
    }
    return false;
}

inline bool isPartialConfigValid(const sst::filtersplusplus::FilterModel &fm, Passband p, Slope s,
                                 DriveMode d)
{
    auto cf = Filter::availableModelConfigurations(fm, true);
    for (auto &c : cf)
    {
        if (get<Passband>(c) == p && get<Slope>(c) == s && get<DriveMode>(c) == d)
            return true;
    }
    return false;
}

inline ModelConfig closestValidModelTo(const sst::filtersplusplus::FilterModel &fm,
                                       const sst::filtersplusplus::ModelConfig &mc)
{
    if (isModelConfigValid(fm, mc))
        return mc;

    // compare all except sm, then dt, then sl, then pb, then return model 0
    auto cf = Filter::availableModelConfigurations(fm);
    for (auto &c : cf)
    {
        if (c.pt == mc.pt && c.st == mc.st && c.dt == mc.dt)
            return c;
    }
    for (auto &c : cf)
    {
        if (c.pt == mc.pt && c.st == mc.st)
            return c;
    }
    for (auto &c : cf)
    {
        if (c.pt == mc.pt)
            return c;
    }
    if (!cf.empty())
        return cf[0];

    return {};
}

template <is_modelconfig_enum T>
inline std::vector<T> potentialValuesFor(const sst::filtersplusplus::FilterModel &fm)
{
    std::set<T> vals;
    auto cf = Filter::availableModelConfigurations(fm, true);
    for (auto &c : cf)
    {
        auto en = get<T>(c);
        if (en != T::UNSUPPORTED)
            vals.insert(get<T>(c));
    }
    return std::vector(vals.begin(), vals.end());
}

namespace details
{
inline bool mcequal(const ModelConfig &mc) { return true; }

template <is_modelconfig_enum T, typename... Args>
    requires(is_modelconfig_enum<Args> && ...)
inline bool mcequal(const ModelConfig &mc, T val, Args... rest_args)
{
    return get<T>(mc) == val && mcequal(mc, rest_args...);
}

template <is_modelconfig_enum T, typename... Args>
    requires(is_modelconfig_enum<Args> && ...)
inline std::vector<std::pair<T, bool>> vavpc(const FilterModel &fm, Args... cstr)
{

    auto cf = Filter::availableModelConfigurations(fm, true);
    auto ac = potentialValuesFor<T>(fm);
    std::map<T, bool> tmp;
    for (auto c : ac)
        tmp[c] = false;

    for (const auto &c : cf)
    {
        if (mcequal(c, cstr...))
        {
            tmp[get<T>(c)] = true;
        }
    }

    std::vector<std::pair<T, bool>> res;
    for (const auto &[t, b] : tmp)
        res.emplace_back(t, b);
    return res;
}

template <is_modelconfig_enum T, typename... Args>
    requires(is_modelconfig_enum<Args> && ...)
inline std::vector<T> vapc(const FilterModel &fm, Args... cstr)
{
    auto t = vavpc<T>(fm, cstr...);
    std::vector<T> res;
    for (auto &[c, b] : t)
        if (b)
            res.emplace_back(c);
    return res;
}
} // namespace details

template <is_modelconfig_enum T, typename... Args>
    requires(is_modelconfig_enum<Args> && ...)
inline bool isPartialMatch(const sst::filtersplusplus::ModelConfig &mc, Args... cstr)
{
    return details::mcequal(mc, cstr...);
}

template <is_modelconfig_enum T>
inline std::vector<std::pair<T, bool>> valuesAndValidityForPartialConfig(const FilterModel &fm)
{
    auto r = potentialValuesFor<T>(fm);
    std::vector<std::pair<T, bool>> res;
    for (const auto &e : r)
        res.emplace_back(e, true);
    return res;
}

template <is_modelconfig_enum T>
inline std::vector<std::pair<T, bool>> valuesAndValidityForPartialConfig(const FilterModel &fm,
                                                                         Passband p)
{
    static_assert(!std::is_same_v<T, Passband>);
    return details::vavpc<T>(fm, p);
}

template <is_modelconfig_enum T>
inline std::vector<std::pair<T, bool>> valuesAndValidityForPartialConfig(const FilterModel &fm,
                                                                         Passband p, Slope s)
{
    static_assert(std::is_same_v<T, FilterSubModel> || std::is_same_v<T, DriveMode>);
    return details::vavpc<T>(fm, p, s);
}

template <is_modelconfig_enum T>
inline std::vector<std::pair<T, bool>>
valuesAndValidityForPartialConfig(const FilterModel &fm, Passband p, Slope s, DriveMode d)
{
    static_assert(std::is_same_v<T, FilterSubModel>);
    return details::vavpc<T>(fm, p, s, d);
}

template <is_modelconfig_enum T, typename... Args>
    requires(is_distinct_modelconfig_enum<Args, T> && ...)
std::vector<std::pair<T, bool>> valuesAndValidityForPartiaulConfig(const FilterModel &fm,
                                                                   Args... args)
{
    return details::vavpc<T>(fm, args...);
}

template <is_modelconfig_enum T> inline std::vector<T> valuesForPartialConfig(const FilterModel &fm)
{
    return details::vapc<T>(fm);
}

template <is_modelconfig_enum T>
inline std::vector<T> valuesForPartialConfig(const FilterModel &fm, const Passband &p)
{
    return details::vapc<T>(fm, p);
}

template <is_modelconfig_enum T>
inline std::vector<T> valuesForPartialConfig(const FilterModel &fm, const Passband &p,
                                             const Slope &s)
{
    return details::vapc<T>(fm, p, s);
}

template <is_modelconfig_enum T>
inline std::vector<T> valuesForPartialConfig(const FilterModel &fm, const Passband &p,
                                             const Slope &s, const DriveMode &d)
{
    return details::vapc<T>(fm, p, s, d);
}

template <is_modelconfig_enum T, typename... Args>
    requires(is_distinct_modelconfig_enum<Args, T> && ...)
std::vector<T> valuesForPartialConfig(const FilterModel &fm, Args... args)
{
    return details::vapc<T>(fm, args...);
}

template <is_modelconfig_enum T, typename... Args>
    requires(is_distinct_modelconfig_enum<Args, T> && ...)
bool noChoicesOrOnlyUnsupported(const FilterModel &fm, Args... args)
{
    auto tmp = valuesForPartialConfig<T>(fm, args...);
    if (tmp.empty())
        return true;
    if (tmp.size() == 1 && tmp[0] == T::UNSUPPORTED)
        return true;
    return false;
}

} // namespace sst::filtersplusplus
#endif // TWO_FILTERS_CONFIGURATION_SELECTOR_IMPL_H
