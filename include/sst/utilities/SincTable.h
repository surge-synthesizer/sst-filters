#ifndef SST_FILTERS_SINCTABLE_H
#define SST_FILTERS_SINCTABLE_H

#include <cmath>

namespace sst::filters::utilities
{

/** Lookup table for evaluating the Sinc function */
struct SincTable
{
    static constexpr int FIRipol_M = 256;
    static constexpr int FIRipol_M_bits = 8;
    static constexpr int FIRipol_N = 12;
    static constexpr int FIRoffset = FIRipol_N >> 1;
    static constexpr int FIRipolI16_N = 8;
    static constexpr int FIRoffsetI16 = FIRipolI16_N >> 1;

    float sinctable alignas(16)[(FIRipol_M + 1) * FIRipol_N * 2]{};
    float sinctable1X alignas(16)[(FIRipol_M + 1) * FIRipol_N]{};
    short sinctableI16 alignas(16)[(FIRipol_M + 1) * FIRipolI16_N]{};

    static inline double sincf(double x)
    {
        if (x == 0)
            return 1;
        return (sin(M_PI * x)) / (M_PI * x);
    }

    static inline double symmetric_blackman(double i, int n)
    {
        // if (i>=n) return 0;
        i -= (n / 2);
        return (0.42 - 0.5 * cos(2 * M_PI * i / (n)) + 0.08 * cos(4 * M_PI * i / (n)));
    }

    SincTable()
    {
        float cutoff = 0.455f;
        float cutoff1X = 0.85f;
        float cutoffI16 = 1.0f;
        int j;
        for (j = 0; j < FIRipol_M + 1; j++)
        {
            for (int i = 0; i < FIRipol_N; i++)
            {
                double t =
                    -double(i) + double(FIRipol_N / 2.0) + double(j) / double(FIRipol_M) - 1.0;
                double val = (float)(symmetric_blackman(t, FIRipol_N) * cutoff * sincf(cutoff * t));
                double val1X =
                    (float)(symmetric_blackman(t, FIRipol_N) * cutoff1X * sincf(cutoff1X * t));
                sinctable[j * FIRipol_N * 2 + i] = (float)val;
                sinctable1X[j * FIRipol_N + i] = (float)val1X;
            }
        }
        for (j = 0; j < FIRipol_M; j++)
        {
            for (int i = 0; i < FIRipol_N; i++)
            {
                sinctable[j * FIRipol_N * 2 + FIRipol_N + i] =
                    (float)((sinctable[(j + 1) * FIRipol_N * 2 + i] -
                             sinctable[j * FIRipol_N * 2 + i]) /
                            65536.0);
            }
        }

        for (j = 0; j < FIRipol_M + 1; j++)
        {
            for (int i = 0; i < FIRipolI16_N; i++)
            {
                double t =
                    -double(i) + double(FIRipolI16_N / 2.0) + double(j) / double(FIRipol_M) - 1.0;
                double val =
                    (float)(symmetric_blackman(t, FIRipolI16_N) * cutoffI16 * sincf(cutoffI16 * t));

                sinctableI16[j * FIRipolI16_N + i] = (short)((float)val * 16384.f);
            }
        }
    }
};

// TODO: is this safe??
const static SincTable globalSincTable;

} // namespace sst::filters::utilities

#endif // SST_FILTERS_SINCTABLE_H
