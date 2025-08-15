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

#ifndef INCLUDE_SST_FILTERS_TUNINGPROVIDER_H
#define INCLUDE_SST_FILTERS_TUNINGPROVIDER_H

#include <algorithm>
#include <cmath>
#include <memory>

namespace sst::filters::detail
{
struct BasicTuningProvider
{
    BasicTuningProvider() = default;

    static constexpr double MIDI_0_FREQ = 8.17579891564371; // or 440.0 * pow( 2.0, - (69.0/12.0 ) )

    enum TuningMode
    {
        RETUNE_ALL = 0,
    };

    TuningMode tuningApplicationMode = RETUNE_ALL;

    static void note_to_omega_ignoring_tuning(float x, float &sinu, float &cosi, float sampleRate)
    {
        auto pitch = note_to_pitch_ignoring_tuning(x);
        auto arg = 2.0f * (float)M_PI * std::min(0.5f, 440.0f * pitch / sampleRate);
        sinu = sin(arg);
        cosi = cos(arg);
    }

    static float note_to_pitch_ignoring_tuning(float x) { return powf(2.f, x * (1.f / 12.f)); }

    static float note_to_pitch_inv_ignoring_tuning(float x)
    {
        return 1.0f / note_to_pitch_ignoring_tuning(x);
    }

    static float note_to_pitch(float x) { return note_to_pitch_ignoring_tuning(x); }

    struct CurrentTuning
    {
        static double logScaledFrequencyForMidiNote(int mn) { return (double)mn / 12.0; }
    } currentTuning;

    struct Patch
    {
        static constexpr bool correctlyTuneCombFilter = true;
    } patch;

    Patch *_patch = &patch;
};
} // namespace sst::filters::detail

#endif // SST_FILTERS_TUNINGPROVIDER_H
