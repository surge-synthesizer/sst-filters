#ifndef SST_FILTERS_TUNINGPROVIDER_H
#define SST_FILTERS_TUNINGPROVIDER_H

#include <algorithm>
#include <cmath>
#include <memory>

namespace sst::filters::detail
{
struct BasicTuningProvider
{
    static constexpr double MIDI_0_FREQ = 8.17579891564371; // or 440.0 * pow( 2.0, - (69.0/12.0 ) )

    enum TuningMode
    {
        RETUNE_ALL = 0,
    };

    TuningMode tuningApplicationMode = RETUNE_ALL;

    BasicTuningProvider() = default;

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
    };
    std::unique_ptr<Patch> _patch = std::make_unique<Patch>();
};
} // namespace sst::filters::detail

#endif // SST_FILTERS_TUNINGPROVIDER_H
