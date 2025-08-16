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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_DETAILS_FILTER_PAYLOAD_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_DETAILS_FILTER_PAYLOAD_H

#include <tuple>
#include <unordered_map>
#include <iostream>
#include <array>
#include "sst/filters.h"

#include "sst/filters++/enums.h"
#include "sst/filters++/enums_to_string.h"
#include "sst/filters++/model_config.h"

namespace sst::filtersplusplus::details
{
struct FilterPayload
{
    FilterPayload() { init(); }
    void setFilterModel(FilterModels model)
    {
        filterModel = model;
        valid = false;
    }
    void setPassType(PassTypes type)
    {
        passType = type;
        valid = false;
    }
    void setSlopeLevel(SlopeLevels slope)
    {
        slopeLevel = slope;
        valid = false;
    }
    void setDriveType(DriveTypes drive)
    {
        driveType = drive;
        valid = false;
    }
    void setSubModelType(SubModelTypes s)
    {
        subModelType = s;
        valid = false;
    }

    void setModelConfiguration(const ModelConfig &k)
    {
        passType = k.pt;
        slopeLevel = k.st;
        driveType = k.dt;
        subModelType = k.mt;
        valid = false;
    }

    ModelConfig getModelConfiguration() const
    {
        return {passType, slopeLevel, driveType, subModelType};
    }

    void setSampleRateAndBlockSize(double sampleRate, size_t blockSize)
    {
        this->sampleRate = sampleRate;
        sampleRateInv = 1.f / sampleRate;
        this->blockSize = blockSize;
        for (auto &m : makers)
            m.setSampleRateAndBlockSize(sampleRate, blockSize);
        qfuState.sampleRate = sampleRate;
        qfuState.sampleRateInv = sampleRateInv;
    }
    bool valid{false};

    std::string displayName() const
    {
        if (!valid)
            return "invalid-configuration";
        return displayName(filterModel, currentModelConfig);
    }

    static std::string displayName(FilterModels f, const ModelConfig &k)
    {
        auto mn = filtersplusplus::toString(f);
        auto cn = k.toString();
        return mn + (cn.empty() ? "" : " ") + cn;
    }

    static std::vector<ModelConfig> availableModelConfigurations(FilterModels m, bool sort = false);

    FilterModels filterModel{FilterModels::Off};
    PassTypes passType{PassTypes::UNSUPPORTED};
    SlopeLevels slopeLevel{SlopeLevels::UNSUPPORTED};
    DriveTypes driveType{DriveTypes::UNSUPPORTED};
    SubModelTypes subModelType{SubModelTypes::UNSUPPORTED};

    double sampleRate{1.}, sampleRateInv{1.};
    size_t blockSize{0};

    std::array<uint32_t, 4> active{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};

    void init() { memset(&qfuState, 0, sizeof(qfuState)); }
    void reset()
    {
        std::fill(qfuState.R, &qfuState.R[sst::filters::n_filter_registers], SIMD_MM(setzero_ps)());
    }

    sst::filters::FilterUnitQFPtr func{nullptr};
    sst::filters::QuadFilterUnitState qfuState;
    std::array<sst::filters::FilterCoefficientMaker<>, 4>
        makers; // later option to externalize this

    using modelConfig_t = sst::filtersplusplus::ModelConfig;
    using legacyType_t = std::pair<sst::filters::FilterType, sst::filters::FilterSubType>;
    using configMap_t = std::unordered_map<modelConfig_t, legacyType_t>;

    legacyType_t resolveLegacyType();

    modelConfig_t currentModelConfig{};
    legacyType_t currentLegacyType{};

    void provideDelayLine(int voice, float *m) { externalDelayLines[voice] = m; }
    std::array<float *, 4> externalDelayLines{};
};

}; // namespace sst::filtersplusplus::details

#include "../models/VemberClassic.h"
#include "../models/VemberLadder.h"
#include "../models/VemberAllPass.h"
#include "../models/K35.h"
#include "../models/VintageLadder.h"
#include "../models/CutoffWarp.h"
#include "../models/ResonanceWarp.h"
#include "../models/DiodeLadder.h"
#include "../models/OBXD_4Pole.h"
#include "../models/OBXD_2Pole.h"
#include "../models/SampleAndHold.h"
#include "../models/Comb.h"
#include "../models/Tripole.h"
#include "../models/CytomicSVF.h"

namespace sst::filtersplusplus::details
{
inline FilterPayload::legacyType_t FilterPayload::resolveLegacyType()
{

    currentModelConfig = modelConfig_t{passType, slopeLevel, driveType, subModelType};

#define FILTER_MODEL_CASE(model, ns)                                                               \
    case model:                                                                                    \
    {                                                                                              \
        namespace ms = ns;                                                                         \
        const auto &s = ms::getModelConfigurations();                                              \
        auto pos = s.find(modelConfig_t{passType, slopeLevel, driveType, subModelType});           \
        if (pos != s.end())                                                                        \
        {                                                                                          \
            currentLegacyType = pos->second;                                                       \
            valid = true;                                                                          \
            return currentLegacyType;                                                              \
        }                                                                                          \
    }                                                                                              \
    break;

    switch (filterModel)
    {
        FILTER_MODEL_CASE(FilterModels::VemberLadder, models::vemberladder);
        FILTER_MODEL_CASE(FilterModels::VemberAllPass, models::vemberallpass);
        FILTER_MODEL_CASE(FilterModels::K35, models::k35);
        FILTER_MODEL_CASE(FilterModels::VemberClassic, models::vemberclassic);
        FILTER_MODEL_CASE(FilterModels::VintageLadder, models::vintageladder);
        FILTER_MODEL_CASE(FilterModels::CutoffWarp, models::cutoffwarp);
        FILTER_MODEL_CASE(FilterModels::ResonanceWarp, models::resonancewarp);
        FILTER_MODEL_CASE(FilterModels::DiodeLadder, models::diodeladder);
        FILTER_MODEL_CASE(FilterModels::OBXD_4Pole, models::obxd_4pole);
        FILTER_MODEL_CASE(FilterModels::OBXD_2Pole, models::obxd_2pole);
        FILTER_MODEL_CASE(FilterModels::SampleAndHold, models::sampleandhold);
        FILTER_MODEL_CASE(FilterModels::Comb, models::comb);
        FILTER_MODEL_CASE(FilterModels::TriPole, models::tripole);
        FILTER_MODEL_CASE(FilterModels::CytomicSVF, models::cytomicsvf);
    default:
        // remove this
        break;
    }

#undef FILTER_MODEL_CASE

    valid = false;
    currentLegacyType =
        legacyType_t{sst::filters::FilterType::fut_none, sst::filters::FilterSubType::st_Standard};
    return currentLegacyType;
}

inline std::vector<ModelConfig> FilterPayload::availableModelConfigurations(FilterModels m,
                                                                            bool sort)
{
#define FILTER_MODEL_CASE(model, ns)                                                               \
    case model:                                                                                    \
    {                                                                                              \
        namespace ms = ns;                                                                         \
        const auto &s = ms::getModelConfigurations();                                              \
        std::vector<ModelConfig> ret;                                                              \
        for (const auto &[k, v] : s)                                                               \
            ret.push_back(k);                                                                      \
        if (sort)                                                                                  \
            std::sort(ret.begin(), ret.end());                                                     \
        return ret;                                                                                \
    }                                                                                              \
    break;

    switch (m)
    {
        FILTER_MODEL_CASE(FilterModels::VemberLadder, models::vemberladder);
        FILTER_MODEL_CASE(FilterModels::VemberAllPass, models::vemberallpass);
        FILTER_MODEL_CASE(FilterModels::K35, models::k35);
        FILTER_MODEL_CASE(FilterModels::VemberClassic, models::vemberclassic);
        FILTER_MODEL_CASE(FilterModels::VintageLadder, models::vintageladder);
        FILTER_MODEL_CASE(FilterModels::CutoffWarp, models::cutoffwarp);
        FILTER_MODEL_CASE(FilterModels::ResonanceWarp, models::resonancewarp);
        FILTER_MODEL_CASE(FilterModels::DiodeLadder, models::diodeladder);
        FILTER_MODEL_CASE(FilterModels::OBXD_4Pole, models::obxd_4pole);
        FILTER_MODEL_CASE(FilterModels::OBXD_2Pole, models::obxd_2pole);
        FILTER_MODEL_CASE(FilterModels::SampleAndHold, models::sampleandhold);
        FILTER_MODEL_CASE(FilterModels::Comb, models::comb);
        FILTER_MODEL_CASE(FilterModels::TriPole, models::tripole);
        FILTER_MODEL_CASE(FilterModels::CytomicSVF, models::cytomicsvf);
    default:
        // remove this
        break;
    }

#undef FILTER_MODEL_CASE

    return {};
}

} // namespace sst::filtersplusplus::details

#endif // FILTER_PAYLOAD_H
