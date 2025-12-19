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
#ifndef INCLUDE_SST_FILTERS_FILTERCOEFFICIENTMAKER_H
#define INCLUDE_SST_FILTERS_FILTERCOEFFICIENTMAKER_H

#include <type_traits>
#include "sst/utilities/globals.h"
#include "FilterConfiguration.h"
#include "TuningProvider.h"

namespace sst
{
namespace filters
{

constexpr int n_cm_coeffs = 8;

/**
 * Class for creating filter coefficients.
 *
 * @tparam TuningProvider   A 12-TET tuning provider is used by default,
 *                          but if you would like to support other tunings,
 *                          you should use a custom tuning provider.
 */
template <typename TuningProvider = detail::BasicTuningProvider> class FilterCoefficientMaker
{
  public:
    /** Default constructor */
    FilterCoefficientMaker();

    /** Sets the sample rate and block size to use for processing the filters */
    void setSampleRateAndBlockSize(float newSampleRate,
                                   int newBlockSize); // @NOTE: for Surge, this should be called
                                                      // with samplerate_os, and BLOCK_SIZE_OS

    /** Resets the coefficients to zero, and the tuning provider to nullptr */
    void Reset();

    /** Creates filter coefficients directly from an array */
    void FromDirect(const float (&N)[n_cm_coeffs]);

    /**
     * Creates filter coefficients for a given set of filter parameters
     *
     * Note that frequency is expected in units of MIDI note number, with A440 = 0.
     */
    void MakeCoeffs(float Freq, float Reso, FilterType Type, FilterSubType SubType,
                    TuningProvider *provider, bool tuningAdjusted, float extra = 0.f,
                    float extra2 = 0.f, float extra3 = 0.f);

    /**
     * Update the coefficients in a filter state.
     * To update the coefficients for a single channel, pass a channel
     * index to the channel argument, otherwise the same coefficients
     * will be used for all channels.
     */
    template <typename StateType> void updateState(StateType &state, int channel = -1);

    /**
     * Update the local coefficients with the coefficients from the filter state.
     * This is necessary since the filter state is responsible for smoothing the filter
     * coefficients.
     */
    template <typename StateType> void updateCoefficients(StateType &state, int channel = 0);

    /** Current filter coefficients */
    float C[n_cm_coeffs]{};

    /** Filter coefficients "delta" to update current coefficients */
    float dC[n_cm_coeffs]{};

    /** "Target" filter coefficients */
    float tC[n_cm_coeffs]{};

    /** Last 'N' value used in fromdirect
     * You would think this would be target (and ultimately it is) but the
     * original usage calculated and applied these over and over and target was
     * and still is the low pass filtered value. So this is that value and if you
     * know that the register calculation is unchanged you can just apply it again
     * in FromDirect (like Filters++::freezeCoefficients does)
     */
    float fromDirectLast[n_cm_coeffs]{};

    static float provider_note_to_pitch(TuningProvider *provider, float note);
    static float provider_note_to_pitch_ignoring_tuning(TuningProvider *provider, float note);
    static float provider_note_to_pitch_inv_ignoring_tuning(TuningProvider *provider, float note);
    static void provider_note_to_omega_ignoring_tuning(TuningProvider *provider, float x,
                                                       float &sinu, float &cosi, float sampleRate);

  private:
    void ToCoupledForm(double A0inv, double A1, double A2, double B0, double B1, double B2,
                       double G);
    void ToNormalizedLattice(double A0inv, double A1, double A2, double B0, double B1, double B2,
                             double G);
    void Coeff_LP12(float Freq, float Reso, int SubType);
    void Coeff_HP12(float Freq, float Reso, int SubType);
    void Coeff_BP12(float Freq, float Reso, int SubType);
    void Coeff_Notch(float Freq, float Reso, int SubType);
    void Coeff_APF(float Freq, float Reso, int SubType);
    void Coeff_LP24(float Freq, float Reso, int SubType);
    void Coeff_HP24(float Freq, float Reso, int SubType);
    void Coeff_BP24(float Freq, float Reso, int SubType);
    void Coeff_LP4L(float Freq, float Reso, int SubType);
    void Coeff_COMB(float Freq, float Reso, int SubType, float cmix = 0.f);
    void Coeff_SNH(float Freq, float Reso, int SubType);
    void Coeff_SVF(float Freq, float Reso, bool);

    bool FirstRun = true;

    TuningProvider *provider = nullptr;

    float sampleRate = 48000.0f;
    float sampleRateInv = 1.0f / sampleRate;

    int blockSize = 32;
    float blockSizeInv = 1.0f / (float)blockSize;
};

} // namespace filters
} // namespace sst

#endif // SST_FILTERS_FILTERCOEFFICIENTMAKER_H
