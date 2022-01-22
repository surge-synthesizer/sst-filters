#ifndef SST_FILTERS_TUNINGPROVIDER_H
#define SST_FILTERS_TUNINGPROVIDER_H

#include <algorithm>
#include <cmath>

namespace sst::filters::detail
{
struct BasicTuningProvider
{
    static constexpr double MIDI_0_FREQ = 8.17579891564371; // or 440.0 * pow( 2.0, - (69.0/12.0 ) )

    enum TuningMode
    {
        RETUNE_NONE,
        RETUNE_ALL,
    };

    TuningMode tuningApplicationMode = RETUNE_ALL;

    BasicTuningProvider() = default;

    static void note_to_omega_ignoring_tuning(float x, float &sinu, float &cosi, float sampleRate)
    {
        auto pitch = powf(2.f, (x - 69.f) * (1.f / 12.f));
        auto arg = 2.0f * (float)M_PI * std::min(0.5f, 440.0f * pitch / sampleRate);
        sinu = sin(arg);
        cosi = cos(arg);
    }

    static float note_to_pitch_ignoring_tuning(float x)
    {
        return powf(2.f, (x - 69.f) * (1.f / 12.f));
    }

    struct CurrentTuning
    {
        static double logScaledFrequencyForMidiNote(int mn) { return (double)mn / 12.0; }
    } currentTuning;
};
} // namespace sst::filters::detail

#endif // SST_FILTERS_TUNINGPROVIDER_H
