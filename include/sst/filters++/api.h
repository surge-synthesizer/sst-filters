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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_API_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_API_H

#include <vector>
#include <cmath>

#include "sst/basic-blocks/simd/setup.h"

#include "enums.h"
#include "details/filter_payload.h"

namespace sst::filtersplusplus
{

/**
 * @brief A class representing the surge filter models with a easier-to-use api
 *
 * The `Filter` class provides an easier to use and enumerated interface on the existing
 * surge filter models, which previously had a direct more-C-style interface and a
 * relatively flat list of feature lookup capabilities. Both APIs are still supported,
 * and the `Filter` api is a wrapper on that more mechanical underlying API.
 *
 * The Filter is configured in a two-part hierarchy, which is specified in `enums.h`.
 *
 * The top of the heirarchy is the `FilterModels` enum. That provides the classes of
 * filters which surge provides. The method `Filter::availableFilterModels()` returns you
 * a list of them.
 *
 * Once you have a model, the model is configured by a tuple of {passtype, slopelevel, drivetype,
 * submodel} each of which is a separate enum. Any of these can be left out (or set to
 * `UNSUPPORTED`). So for instance, the model `FilterModels::VemberClassic` has a configuration
 * `{PassTypes::LP, SlopeLeveLs::Slope_12db, DriveTypes::Clean}`. You can enumerate all the
 * configuration tuples ysing `Filter::availableModelConfigurations()`.
 *
 * Each of the filters is implemented to simultaneously run 4-wide SIMD on a vector of 4 floats in
 * to 4 floats out. While a filter must have a consistent model across the voices, it can have
 * varied resonances and cutoffs, so some apis take a voice parameter.
 *
 * Finally the surge filters work on a two-rate principle, where there is a control rate block at
 * which you can reset coefficients and a sample-rate api. Managing these two rates is possible with
 * the API and is the responsibility of the API caller. At the outset of a block after setting
 * coefficients, call prepareBlock, and at the conclusion of the block, call concludeBlock.
 *
 * As such a typical session could look something like this
 *
 * ```cpp
 *      namespace sfpp = sst::filtersplusplus;
 *      auto filter = sfpp::Filter();
 *
 *      filter.setSampleRateAndBlockSize(48000, 16);
 *
 *      filter.setFilterModel(sfpp::FilterModels::OBXD_4Pole);
 *      filter.setPassType(sfpp::PassTypes::LP);
 *      filter.setSlopeLevel(sfpp::SlopeLevels::Slope_18db);
 *      if (!filter.prepareInstance())
 *          REQUIRE(false);
 *
 *      size_t blockPos{0};
 *      for (int i=0; i<100; ++i)
 *      {
 *          if (blockPos == 0)
 *          {
 *              for (int v=0; v<4; ++v)
 *              {
 *                  filter.makeCoefficients(v, -9, 0.1 + v * 0.2);
 *              }
 *              filter.prepareBlock();
 *          }
 *
 *          auto out = filter.processSample(SIMD_MM(setzero_ps)());
 *
 *          blockPos ++;
 *          if (blockPos == filter.getBlockSize())
 *          {
 *              filter.concludeBlock();
 *              blockPos = 0;
 *          }
 *      }
 * ```
 */
struct Filter
{
    /*
     * These APIs set up the configuration either with model + individual enums
     * or Model + ModeLConfig (which is an object with each of the enums).
     */

    void setFilterModel(FilterModels model) { payload.setFilterModel(model); }
    FilterModels getFilterModel() const { return payload.filterModel; }

    void setPassType(PassTypes type) { payload.setPassType(type); }
    void setSlopeLevel(SlopeLevels slope) { payload.setSlopeLevel(slope); }
    void setDriveType(DriveTypes drive) { payload.setDriveType(drive); }
    void setSubModelType(SubModelTypes smt) { payload.setSubModelType(smt); }

    void setModelConfiguration(const ModelConfig &sk) { payload.setModelConfiguration(sk); }
    ModelConfig getModelConfiguration() const { return payload.getModelConfiguration(); }

    /**
     * A reasonable user facing display name for the given configuration
     */
    std::string displayName() const { return payload.displayName(); }

    /**
     * Some model types require a float buffer for a delay line. We assume this relatively
     * large chunk of memory will be managed outside this object. This api returns 0 if no such
     * buffer is required, and the requires size in 32 big floats if so. Currently only the comb
     * filter models require this.
     *
     * @param model Which model
     * @param k  Configured how
     * @return how many floats must the delay line have
     */
    static size_t requiredDelayLinesSizes(FilterModels model, const ModelConfig &k);

    /**
     * If a delay line is needed, each active voice requires one.
     *
     * @param voice Each voice needs a distinct buffer
     * @param memory This is a pointer to memory which is not owned by this object and must outlive
     * it
     */
    void provideDelayLine(int voice, float *memory) { payload.provideDelayLine(voice, memory); }

    /**
     * Or provide them all in one go
     */
    void provideAllDelayLines(float *memory)
    {
        auto sz = requiredDelayLinesSizes(getFilterModel(), getModelConfiguration());

        payload.provideDelayLine(0, memory);
        payload.provideDelayLine(1, memory + sz);
        payload.provideDelayLine(2, memory + sz * 2);
        payload.provideDelayLine(3, memory + sz * 3);
    }

    size_t getBlockSize() const { return payload.blockSize; }

    /**
     * Some models have coefficient features beyond jsut cutoff and resonance. For instance the
     * OBXD 4 pole low pass has a continous pole morphing mode (SlopeLevels::Slope_Morph). In that
     * case you will need to provide the 'extra' arguments to the makeCoefficients call to get
     * the feature. This function tells you how manu such 'extra' arguments are consumed for
     * a given model/config.
     *
     */
    [[nodiscard]] static int coefficientsExtraCount(FilterModels model, const ModelConfig &c);

    /**
     * Get a list of the available models supported by the API
     */
    static std::vector<FilterModels> availableModels();

    /**
     * For a given model, return the configurations that model supports. The list by
     * default will be unsorted, but the sort option allows you to sort it. Note that this
     * is an allocating API to make that vector so you dont want to use it while processing.
     */
    static std::vector<ModelConfig> availableModelConfigurations(FilterModels model,
                                                                 bool sort = false)
    {
        return details::FilterPayload::availableModelConfigurations(model, sort);
    }

    /**
     * Once a filter has been set up with a model type and a configuration
     * the instance needs preparation to resolve the internal state. This function
     * will do so and will return true on success
     */
    [[nodiscard]] bool prepareInstance();

    /**
     * If a call to prepareInstance is required, this will return true.
     */
    [[nodiscard]] bool requiresPreparation() const { return !payload.valid; }

    /**
     * With a given voice, for a given cutoff and resonance, set the internal coefficient
     * state. This method will allow you to interpolate over blocks so is appropriate for
     * per-block modification of coefficients, but as a result must be called every block
     * before prepareBlock.
     */
    void makeCoefficients(int voice, float cutoff, float resonance, float extra = 0.f,
                          float extra2 = 0.f, float extra3 = 0.f);

    /**
     * With a given voice, for a given cutoff and resonance, set the internal coefficient
     * state. This method will assume the coefficients are fixed and so is appropriate for
     * static filter configurations and only needs to be called once, between prepareInstance
     * and prepareBlock, but is not suitable for dynamically changing cutoff and resonance.
     */
    void makeConstantCoefficients(int voice, float cutoff, float resonance, float extra = 0.f,
                                  float extra2 = 0.f, float extra3 = 0.f);

    /**
     * The filters can have a concept of an inactive voice which for some filters give a
     * moderate CPU advantage in the SIMD pipelines.
     */
    void setActive(int voice, bool b) { payload.active[voice] = b ? 0xFFFFFF : 0; }
    void setMono()
    {
        setActive(0, true);
        setActive(1, false);
        setActive(2, false);
        setActive(3, false);
    }
    void setStereo()
    {
        setActive(0, true);
        setActive(1, true);
        setActive(2, false);
        setActive(3, false);
    }
    void setQuad()
    {
        setActive(0, true);
        setActive(1, true);
        setActive(2, true);
        setActive(3, true);
    }

    void setSampleRateAndBlockSize(double sampleRate, size_t blockSize)
    {
        payload.setSampleRateAndBlockSize(sampleRate, blockSize);
    }

    /**
     * prepareBlock must be called after coeffients are set but at the start of each
     * block, as set by blockSize in setSampleRateAndBlockSize
     */
    void prepareBlock();
    /**
     * once prepare block has been called, you must call processSample exactly blockSize
     * times, mapping the vector value in to the vector value out. Convenience functions for
     * single float and dual float are below.
     */
    SIMD_M128 processSample(SIMD_M128 in);
    /**
     * At the end of blockSize samples, and before resetting coefficients or the next call to
     * prepareBlock, you need to call concludeBlock
     */
    void concludeBlock();

    /**
     * convenience functions for mono channel if you dont want to manage simd
     */
    float processMonoSample(float in);

    /**
     * convenience functions for stereo channel if you dont want to manage simd
     */
    void processStereoSample(float inL, float inR, float &outL, float &outR);

    /**
     * convenience functions for quad channel if you dont want to manage simd.
     * But probably you are better biting the bullet and managing the simd outside
     */
    void processQuadSample(float in[4], float out[4]);

    /**
     * Initializes the filter state
     */
    void init() { payload.init(); }

    /**
     * Resets the filter registers; leaves the rest of the state intact
     */
    void reset() { payload.reset(); }

  protected:
    details::FilterPayload payload;
};
} // namespace sst::filtersplusplus

#include "details/filter_impl.h"

#endif // API_H
