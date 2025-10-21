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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_CONFIGURATION_SELECTOR_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_CONFIGURATION_SELECTOR_H

#include "api.h"
#include <set>
#include <map>

/**
 * configuration_selector contains a set of useful functions to genreate uis
 * and allow users to navigate the configuration space more naturally than the
 * simple flat list in the core api.
 *
 * To make this easier, we have editorialized a hierarchy of enums,
 * PassBand > Slope > Drive > FilterSubModel. That is, the 'partical config
 * validity' means you check passband first and so on.
 *
 * You could make a different choice of course! Its just tweaking the presented
 * API. The internal stuff in the impl/details is just all vararg packs anyway
 */
namespace sst::filtersplusplus
{

/**
 * A simple boolean check of if a given model config is valid for a given model
 * @param fm A given model
 * @param tc A candidate config
 */
bool isModelConfigValid(const FilterModel &fm, const ModelConfig &tc);

/**
 * This API allows you to build a partial config and test if you are valid
 * so far. So for a model, do you have a valid passband, passband/slope
 * and passband/slope/drive. The full quartet can be answered with isModelConfigValid
 */
bool isPartialConfigValid(const FilterModel &fm, Passband p);
bool isPartialConfigValid(const FilterModel &fm, Passband p, Slope s);
bool isPartialConfigValid(const FilterModel &fm, Passband p, Slope s, DriveMode d);

/**
 * isPartialMatch is just a utility where you can send a model config and
 * then any subset of enums and get a true/false.
 */
template <is_modelconfig_enum T, typename... Args>
    requires(is_modelconfig_enum<Args> && ...)
bool isPartialMatch(const ModelConfig &mc, Args... cstr);

/**
 * Given a model config, which is the 'closes' valid model to the one
 * handed in. If the model config is valid, it is just returned, otherwise
 * find partial matches up the stack dropping SM, DT, Slp, and Filt each.
 */
ModelConfig closestValidModelTo(const FilterModel &fm, const ModelConfig &mc);

/**
 * Given an enum type, what are the potential values this
 * model could support for just that dimension. So like
 * potentialValuesFor<sst::filtersplusplus::Slope>(fm)
 * would give you all the slopes that any configuration
 * of the model would use.
 */
template <is_modelconfig_enum T>
std::vector<T> potentialValuesFor(const FilterModel &fm, bool returnUnsupportedIfEmpty = false);

/**
 * If a model doesn't use a particular dimension at all
 * (namely all values of that dimension are UNSUPPORTED)
 * return true.
 */
template <is_modelconfig_enum T> inline bool supportsChoice(const FilterModel &fm)
{
    return !potentialValuesFor<T>(fm).empty();
}

/**
 * To probe a partial config, you want to know which values are possible valid
 * and either get a list with only valid ones, or get a list of values with validity.
 * This first api provides the second - a pair of values and bools for a partiaul
 * config in one dimension along the second dimension.
 */
template <is_modelconfig_enum T>
std::vector<std::pair<T, bool>> valuesAndValidityForPartialConfig(const FilterModel &fm);
template <is_modelconfig_enum T>
std::vector<std::pair<T, bool>> valuesAndValidityForPartialConfig(const FilterModel &fm,
                                                                  Passband p);
template <is_modelconfig_enum T>
std::vector<std::pair<T, bool>> valuesAndValidityForPartialConfig(const FilterModel &fm, Passband p,
                                                                  Slope s);
template <is_modelconfig_enum T>
std::vector<std::pair<T, bool>> valuesAndValidityForPartialConfig(const FilterModel &fm, Passband p,
                                                                  Slope s, DriveMode d);

/*
 * This vararg version exists so you can use any ordering you want of sub-model
 */
template <is_modelconfig_enum T, typename... Args>
    requires(is_distinct_modelconfig_enum<Args, T> && ...)
std::vector<std::pair<T, bool>> valuesAndValidityForPartiaulConfig(const FilterModel &fm,
                                                                   Args... args);

/**
 * And this provides the filtered version, so items in the above api with false here
 * would just not appear in the list.
 */
template <is_modelconfig_enum T> std::vector<T> valuesForPartialConfig(const FilterModel &fm);
template <is_modelconfig_enum T>
std::vector<T> valuesForPartialConfig(const FilterModel &fm, const Passband &p);
template <is_modelconfig_enum T>
std::vector<T> valuesForPartialConfig(const FilterModel &fm, const Passband &p, const Slope &s);
template <is_modelconfig_enum T>
std::vector<T> valuesForPartialConfig(const FilterModel &fm, const Passband &p, const Slope &s,
                                      const DriveMode &d);

/*
 * This vararg version exists so you can use any ordering you want of sub-model
 */
template <is_modelconfig_enum T, typename... Args>
    requires(is_distinct_modelconfig_enum<Args, T> && ...)
std::vector<T> valuesForPartialConfig(const FilterModel &fm, Args... args);

/*
 * This is a utility telling if there's either no values or just unsupported valid
 */
template <is_modelconfig_enum T, typename... Args>
    requires(is_distinct_modelconfig_enum<Args, T> && ...)
bool noChoicesOrOnlyUnsupported(const FilterModel &fm, Args... args);

} // namespace sst::filtersplusplus

#include "details/configuration_selector_impl.h"

#endif // FILTERS_PLUS_PLUS_CONFIGURATION_SELECTOR_H
