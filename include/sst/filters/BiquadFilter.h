/*
 * sst-filters - A header-only collection of SIMD filter
 * implementations by the Surge Synth Team
 *
 * Copyright 2019-2024, various authors, as described in the GitHub
 * transaction log.
 *
 * sst-filters is released under the Gnu General Public Licens
 * version 3 or later. Some of the filters in this package
 * originated in the version of Surge open sourced in 2018.
 *
 * All source in sst-filters available at
 * https://github.com/surge-synthesizer/sst-filters
 */
#ifndef INCLUDE_SST_FILTERS_BIQUADFILTER_H
#define INCLUDE_SST_FILTERS_BIQUADFILTER_H

#include <utility>
#include <algorithm>
#include <complex>

#include <concepts>

#include "sst/basic-blocks/concepts/concepts.h"

namespace sst::filters::Biquad
{
template <typename D, size_t BLOCK_SIZE>
concept ValidBiquad = sst::basic_blocks::concepts::is_power_of_two_ge(BLOCK_SIZE, 4UL) &&
                      sst::basic_blocks::concepts::providesNoteToPitch<D> &&
                      sst::basic_blocks::concepts::providesDbToLinear<D> &&
                      sst::basic_blocks::concepts::supportsSampleRateInv<D>;

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
struct alignas(16) BiquadFilter
{
  private:
    static constexpr double minBW = 0.0001;
    static constexpr double d_lp = 0.004;
    static constexpr double d_lpinv = 1.0 - 0.004;

    union vdouble
    {
        double d[2];
    };
    class vlag
    {
      public:
        vdouble v alignas(16), target_v alignas(16);
        vlag() {}
        void init_x87()
        {
            v.d[0] = 0;
            v.d[1] = 0;
            target_v.d[0] = 0;
            target_v.d[1] = 0;
        }

        inline void process() { v.d[0] = v.d[0] * d_lpinv + target_v.d[0] * d_lp; }
        inline void newValue(double f) { target_v.d[0] = f; }
        inline void instantize() { v = target_v; }
        inline void startValue(double f)
        {
            target_v.d[0] = f;
            v.d[0] = f;
        }
    };

    vlag a1 alignas(16), a2 alignas(16), b0 alignas(16), b1 alignas(16), b2 alignas(16);
    vdouble reg0 alignas(16), reg1 alignas(16);

    inline void flush_denormal(double &d)
    {
        if (fabs(d) < 1E-30)
        {
            d = 0;
        }
    }

  public:
    BiquadFilter(D *d = nullptr);
    void coeff_LP(double omega, double Q);
    /** Compared to coeff_LP, this version adds a small boost at high frequencies */
    void coeff_LP2B(double omega, double Q);
    void coeff_HP(double omega, double Q);
    void coeff_BP(double omega, double Q);
    void coeff_LP_with_BW(double omega, double BW);
    void coeff_HP_with_BW(double omega, double BW);
    void coeff_BP2A(double omega, double Q);
    void coeff_PKA(double omega, double Q);
    void coeff_NOTCH(double omega, double Q);
    void coeff_peakEQ(double omega, double BW, double gain);
    void coeff_LPHPmorph(double omega, double Q, double morph);
    void coeff_APF(double omega, double Q);
    void coeff_orfanidisEQ(double omega, double BW, double pgaindb, double bgaindb, double zgain);
    void coeff_same_as_last_time();
    void coeff_instantize();

    void process_block(float *data);
    // void process_block_SSE2(float *data);
    void process_block(float *dataL, float *dataR);
    // void process_block_SSE2(float *dataL,float *dataR);
    void process_block_to(float *, float *);
    void process_block_to(float *dataL, float *dataR, float *dstL, float *dstR);
    // void process_block_to_SSE2(float *dataL,float *dataR, float *dstL,float *dstR);
    void process_block_slowlag(float *dataL, float *dataR);
    // void process_block_slowlag_SSE2(float *dataL,float *dataR);
    void process_block(double *data);
    // void process_block_SSE2(double *data);

    inline float process_sample(float input)
    {
        a1.process();
        a2.process();
        b0.process();
        b1.process();
        b2.process();

        double op;

        op = input * b0.v.d[0] + reg0.d[0];
        reg0.d[0] = input * b1.v.d[0] - a1.v.d[0] * op + reg1.d[0];
        reg1.d[0] = input * b2.v.d[0] - a2.v.d[0] * op;

        return (float)op;
    }

    inline void process_sample(float L, float R, float &lOut, float &rOut)
    {
        a1.process();
        a2.process();
        b0.process();
        b1.process();
        b2.process();

        double op;
        double input = L;
        op = input * b0.v.d[0] + reg0.d[0];
        reg0.d[0] = input * b1.v.d[0] - a1.v.d[0] * op + reg1.d[0];
        reg1.d[0] = input * b2.v.d[0] - a2.v.d[0] * op;

        lOut = op;

        input = R;
        op = input * b0.v.d[0] + reg0.d[1];
        reg0.d[1] = input * b1.v.d[0] - a1.v.d[0] * op + reg1.d[1];
        reg1.d[1] = input * b2.v.d[0] - a2.v.d[0] * op;

        rOut = op;
    }

    inline void process_sample_nolag(float &L, float &R)
    {
        double op;

        op = L * b0.v.d[0] + reg0.d[0];
        reg0.d[0] = L * b1.v.d[0] - a1.v.d[0] * op + reg1.d[0];
        reg1.d[0] = L * b2.v.d[0] - a2.v.d[0] * op;
        L = (float)op;

        op = R * b0.v.d[0] + reg0.d[1];
        reg0.d[1] = R * b1.v.d[0] - a1.v.d[0] * op + reg1.d[1];
        reg1.d[1] = R * b2.v.d[0] - a2.v.d[0] * op;
        R = (float)op;
    }

    inline void process_sample_nolag(float &L, float &R, float &Lout, float &Rout)
    {
        double op;

        op = L * b0.v.d[0] + reg0.d[0];
        reg0.d[0] = L * b1.v.d[0] - a1.v.d[0] * op + reg1.d[0];
        reg1.d[0] = L * b2.v.d[0] - a2.v.d[0] * op;
        Lout = (float)op;

        op = R * b0.v.d[0] + reg0.d[1];
        reg0.d[1] = R * b1.v.d[0] - a1.v.d[0] * op + reg1.d[1];
        reg1.d[1] = R * b2.v.d[0] - a2.v.d[0] * op;
        Rout = (float)op;
    }

    inline void process_sample_nolag_noinput(float &Lout, float &Rout)
    {
        double op;

        op = reg0.d[0];
        reg0.d[0] = -a1.v.d[0] * op + reg1.d[0];
        reg1.d[0] = -a2.v.d[0] * op;
        Lout = (float)op;

        op = reg0.d[1];
        reg0.d[1] = -a1.v.d[0] * op + reg1.d[1];
        reg1.d[1] = -a2.v.d[0] * op;
        Rout = (float)op;
    }

    // static double calc_omega(double scfreq){ return (2*3.14159265358979323846) * min(0.499,
    // 440*powf(2,scfreq)*samplerate_inv); }
    double calc_omega(double scfreq)
    {
        return (2 * 3.14159265358979323846) * 440 *
               sst::basic_blocks::concepts::convertNoteToPitch(storage, (float)(12.f * scfreq)) *
               sst::basic_blocks::concepts::getSampleRateInv(storage);
    }
    double calc_omega_from_Hz(double Hz)
    {
        return (2 * 3.14159265358979323846) * Hz *
               sst::basic_blocks::concepts::getSampleRateInv(storage);
    }
    double calc_v1_Q(double reso) { return 1 / (1.02 - std::clamp(reso, 0.0, 1.0)); }
    // inline void process_block_stereo(float *dataL,float *dataR);
    // inline void process_block(double *data);
    // inline double process_sample(double sample);
    void setBlockSize(int bs);
    void suspend()
    {
        reg0.d[0] = 0;
        reg1.d[0] = 0;
        reg0.d[1] = 0;
        reg1.d[1] = 0;
        first_run = true;
        a1.init_x87();
        a2.init_x87();
        b0.init_x87();
        b1.init_x87();
        b2.init_x87();
    }

    float plot_magnitude(float f);
    D *storage{nullptr};

  protected:
    void set_coef(double a0, double a1, double a2, double b0, double b1, double b2);
    bool first_run;
};

inline double square(double x) { return x * x; }

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline BiquadFilter<D, BLOCK_SIZE>::BiquadFilter(D *d) : storage(d)
{
    reg0.d[0] = 0;
    reg1.d[0] = 0;
    reg0.d[1] = 0;
    reg1.d[1] = 0;
    first_run = true;

    {
        a1.init_x87();
        a2.init_x87();
        b0.init_x87();
        b1.init_x87();
        b2.init_x87();
    }
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::coeff_LP(double omega, double Q)
{
    if (omega > M_PI)
        set_coef(1, 0, 0, 1, 0, 0);
    else
    {
        double cosi = cos(omega), sinu = sin(omega), alpha = sinu / (2 * Q), b0 = (1 - cosi) * 0.5,
               b1 = 1 - cosi, b2 = (1 - cosi) * 0.5, a0 = 1 + alpha, a1 = -2 * cosi, a2 = 1 - alpha;

        set_coef(a0, a1, a2, b0, b1, b2);
    }
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::coeff_LP2B(double omega, double Q)
{
    if (omega > M_PI)
        set_coef(1, 0, 0, 1, 0, 0);
    else
    {
        double w_sq = omega * omega;
        double den =
            (w_sq * w_sq) + (M_PI * M_PI * M_PI * M_PI) + w_sq * (M_PI * M_PI) * (1 / Q - 2);
        double G1 = std::min(1.0, sqrt((w_sq * w_sq) / den) * 0.5);

        double cosi = cos(omega), sinu = sin(omega), alpha = sinu / (2 * Q),

               A = 2 * sqrt(G1) * sqrt(2 - G1), b0 = (1 - cosi + G1 * (1 + cosi) + A * sinu) * 0.5,
               b1 = (1 - cosi - G1 * (1 + cosi)),
               b2 = (1 - cosi + G1 * (1 + cosi) - A * sinu) * 0.5, a0 = (1 + alpha), a1 = -2 * cosi,
               a2 = 1 - alpha;

        set_coef(a0, a1, a2, b0, b1, b2);
    }
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::coeff_HP(double omega, double Q)
{
    if (omega > M_PI)
        set_coef(1, 0, 0, 0, 0, 0);
    else
    {
        double cosi = cos(omega), sinu = sin(omega), alpha = sinu / (2 * Q), b0 = (1 + cosi) * 0.5,
               b1 = -(1 + cosi), b2 = (1 + cosi) * 0.5, a0 = 1 + alpha, a1 = -2 * cosi,
               a2 = 1 - alpha;

        set_coef(a0, a1, a2, b0, b1, b2);
    }
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::coeff_BP(double omega, double Q)
{
    double cosi = cos(omega), sinu = sin(omega), alpha = sinu / (2.0 * Q), b0 = alpha, b2 = -alpha,
           a0 = 1 + alpha, a1 = -2 * cosi, a2 = 1 - alpha;

    set_coef(a0, a1, a2, b0, 0, b2);
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::coeff_BP2A(double omega, double BW)
{
    double cosi = cos(omega), sinu = sin(omega), q = 1 / (0.02 + 30 * BW * BW),
           alpha = sinu / (2 * q), b0 = alpha, b2 = -alpha, a0 = 1 + alpha, a1 = -2 * cosi,
           a2 = 1 - alpha;

    set_coef(a0, a1, a2, b0, 0, b2);
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::coeff_PKA(double omega, double QQ)
{
    double cosi = cos(omega), sinu = sin(omega), reso = std::clamp(QQ, 0.0, 1.0),
           q = 0.1 + 10 * reso * reso, alpha = sinu / (2 * q), b0 = q * alpha, b2 = -q * alpha,
           a0 = 1 + alpha, a1 = -2 * cosi, a2 = 1 - alpha;

    set_coef(a0, a1, a2, b0, 0, b2);
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::coeff_NOTCH(double omega, double QQ)
{
    if (omega > M_PI)
        set_coef(1, 0, 0, 1, 0, 0);
    else
    {
        double cosi = cos(omega), sinu = sin(omega), reso = std::clamp(QQ, 0.0, 1.0),
               q = 1 / (0.02 + 30 * reso * reso), alpha = sinu / (2 * q), b0 = 1.0,
               b1 = -2.0 * cosi, b2 = 1.0, a0 = 1.0 + alpha, a1 = -2.0 * cosi, a2 = 1.0 - alpha;

        set_coef(a0, a1, a2, b0, b1, b2);
    }
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::coeff_LP_with_BW(double omega, double BW)
{
    coeff_LP(omega, 1 / BW);
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::coeff_HP_with_BW(double omega, double BW)
{
    coeff_HP(omega, 1 / BW);
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::coeff_LPHPmorph(double omega, double Q, double morph)
{
    double HP = std::clamp(morph, 0.0, 1.0), LP = 1 - HP; // , BP = LP * HP;
    HP *= HP;
    LP *= LP;

    double cosi = cos(omega), sinu = sin(omega), alpha = sinu / (2 * Q), b0 = alpha, b1 = 0,
           b2 = -alpha, a0 = 1 + alpha, a1 = -2 * cosi, a2 = 1 - alpha;

    set_coef(a0, a1, a2, b0, b1, b2);
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::coeff_APF(double omega, double Q)
{
    if ((omega < 0.0) || (omega > M_PI))
        set_coef(1, 0, 0, 1, 0, 0);
    else
    {
        double cosi = cos(omega), sinu = sin(omega), alpha = sinu / (2 * Q), b0 = (1 - alpha),
               b1 = -2 * cosi, b2 = (1 + alpha), a0 = (1 + alpha), a1 = -2 * cosi, a2 = (1 - alpha);

        set_coef(a0, a1, a2, b0, b1, b2);
    }
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::coeff_peakEQ(double omega, double BW, double gain)
{
    coeff_orfanidisEQ(omega, BW, sst::basic_blocks::concepts::convertDbToLinear(storage, gain),
                      sst::basic_blocks::concepts::convertDbToLinear(storage, gain * 0.5), 1);
}

template <typename DT, size_t BLOCK_SIZE>
    requires(ValidBiquad<DT, BLOCK_SIZE>)
inline void BiquadFilter<DT, BLOCK_SIZE>::coeff_orfanidisEQ(double omega, double BW, double G,
                                                            double GB, double G0)
{
    // For the curious http://eceweb1.rutgers.edu/~orfanidi/ece521/hpeq.pdf appears to be the source
    // of this
    // double limit = 0.95;
    double w0 = omega; // min(M_PI-0.000001,omega);
    BW = std::max(minBW, BW);
    double Dww = 2 * w0 * sinh((log(2.0) / 2.0) * BW); // sinh = (e^x - e^-x)/2

    // double gainscale = 1;
    // if(omega>M_PI) gainscale = 1 / (1 + (omega-M_PI)*(4/Dw));

    if (abs(G - G0) > 0.00001)
    {
        double F = abs(G * G - GB * GB);
        double G00 = abs(G * G - G0 * G0);
        double F00 = abs(GB * GB - G0 * G0);
        double num =
            G0 * G0 * square(w0 * w0 - (M_PI * M_PI)) + G * G * F00 * (M_PI * M_PI) * Dww * Dww / F;
        double den = square(w0 * w0 - M_PI * M_PI) + F00 * M_PI * M_PI * Dww * Dww / F;
        double G1 = sqrt(num / den);

        if (omega > M_PI)
        {
            G = G1 * 0.9999;
            w0 = M_PI - 0.00001;
            G00 = abs(G * G - G0 * G0);
            F00 = abs(GB * GB - G0 * G0);
        }

        double G01 = abs(G * G - G0 * G1);
        double G11 = abs(G * G - G1 * G1);
        double F01 = abs(GB * GB - G0 * G1);
        double F11 = abs(GB * GB - G1 * G1); // goes crazy (?)
        double W2 = sqrt(G11 / G00) * square(tan(w0 / 2));
        double w_lower = w0 * powf(2, -0.5 * BW);
        double w_upper =
            2 * atan(sqrt(F00 / F11) * sqrt(G11 / G00) * square(tan(w0 / 2)) / tan(w_lower / 2));
        double Dw = abs(w_upper - w_lower);
        double DW = (1 + sqrt(F00 / F11) * W2) * tan(Dw / 2);

        double C = F11 * DW * DW - 2 * W2 * (F01 - sqrt(F00 * F11));
        double D = 2 * W2 * (G01 - sqrt(G00 * G11));
        double A = sqrt((C + D) / F);
        double B = sqrt((G * G * C + GB * GB * D) / F);
        double a0 = (1 + W2 + A), a1 = -2 * (1 - W2), a2 = (1 + W2 - A), b0 = (G1 + G0 * W2 + B),
               b1 = -2 * (G1 - G0 * W2), b2 = (G1 - B + G0 * W2);

        set_coef(a0, a1, a2, b0, b1, b2);
    }
    else
    {
        set_coef(1, 0, 0, 1, 0, 0);
    }
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::coeff_same_as_last_time()
{
    // If you want to change interpolation then set dv = 0 here
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::coeff_instantize()
{
    a1.instantize();
    a2.instantize();
    b0.instantize();
    b1.instantize();
    b2.instantize();
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::set_coef(double a0, double a1, double a2, double b0,
                                                  double b1, double b2)
{
    double a0inv = 1 / a0;

    b0 *= a0inv;
    b1 *= a0inv;
    b2 *= a0inv;
    a1 *= a0inv;
    a2 *= a0inv;
    if (first_run)
    {
        this->a1.startValue(a1);
        this->a2.startValue(a2);
        this->b0.startValue(b0);
        this->b1.startValue(b1);
        this->b2.startValue(b2);
        first_run = false;
    }
    this->a1.newValue(a1);
    this->a2.newValue(a2);
    this->b0.newValue(b0);
    this->b1.newValue(b1);
    this->b2.newValue(b2);
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::process_block(float *data)
{
    {
        int k;
        for (k = 0; k < BLOCK_SIZE; k++)
        {
            a1.process();
            a2.process();
            b0.process();
            b1.process();
            b2.process();

            double input = data[k];
            double op;

            op = input * b0.v.d[0] + reg0.d[0];
            reg0.d[0] = input * b1.v.d[0] - a1.v.d[0] * op + reg1.d[0];
            reg1.d[0] = input * b2.v.d[0] - a2.v.d[0] * op;

            data[k] = op;
        }
        flush_denormal(reg0.d[0]);
        flush_denormal(reg1.d[0]);
    }
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::process_block_to(float *__restrict data,
                                                          float *__restrict dataout)
{
    {
        int k;
        for (k = 0; k < BLOCK_SIZE; k++)
        {
            a1.process();
            a2.process();
            b0.process();
            b1.process();
            b2.process();

            double input = data[k];
            double op;

            op = input * b0.v.d[0] + reg0.d[0];
            reg0.d[0] = input * b1.v.d[0] - a1.v.d[0] * op + reg1.d[0];
            reg1.d[0] = input * b2.v.d[0] - a2.v.d[0] * op;

            dataout[k] = op;
        }
        flush_denormal(reg0.d[0]);
        flush_denormal(reg1.d[0]);
    }
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::process_block_slowlag(float *__restrict dataL,
                                                               float *__restrict dataR)
{
    {
        a1.process();
        a2.process();
        b0.process();
        b1.process();
        b2.process();

        int k;
        for (k = 0; k < BLOCK_SIZE; k++)
        {
            double input = dataL[k];
            double op;

            op = input * b0.v.d[0] + reg0.d[0];
            reg0.d[0] = input * b1.v.d[0] - a1.v.d[0] * op + reg1.d[0];
            reg1.d[0] = input * b2.v.d[0] - a2.v.d[0] * op;

            dataL[k] = op;

            input = dataR[k];
            op = input * b0.v.d[0] + reg0.d[1];
            reg0.d[1] = input * b1.v.d[0] - a1.v.d[0] * op + reg1.d[1];
            reg1.d[1] = input * b2.v.d[0] - a2.v.d[0] * op;

            dataR[k] = op;
        }
        flush_denormal(reg0.d[0]);
        flush_denormal(reg1.d[0]);
        flush_denormal(reg0.d[1]);
        flush_denormal(reg1.d[1]);
    }
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::process_block(float *dataL, float *dataR)
{
    {
        int k;
        for (k = 0; k < BLOCK_SIZE; k++)
        {
            a1.process();
            a2.process();
            b0.process();
            b1.process();
            b2.process();

            double input = dataL[k];
            double op;

            op = input * b0.v.d[0] + reg0.d[0];
            reg0.d[0] = input * b1.v.d[0] - a1.v.d[0] * op + reg1.d[0];
            reg1.d[0] = input * b2.v.d[0] - a2.v.d[0] * op;

            dataL[k] = op;

            input = dataR[k];
            op = input * b0.v.d[0] + reg0.d[1];
            reg0.d[1] = input * b1.v.d[0] - a1.v.d[0] * op + reg1.d[1];
            reg1.d[1] = input * b2.v.d[0] - a2.v.d[0] * op;

            dataR[k] = op;
        }
        flush_denormal(reg0.d[0]);
        flush_denormal(reg1.d[0]);
        flush_denormal(reg0.d[1]);
        flush_denormal(reg1.d[1]);
    }
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::process_block_to(float *dataL, float *dataR, float *dstL,
                                                          float *dstR)
{
    {
        int k;
        for (k = 0; k < BLOCK_SIZE; k++)
        {
            a1.process();
            a2.process();
            b0.process();
            b1.process();
            b2.process();

            double input = dataL[k];
            double op;

            op = input * b0.v.d[0] + reg0.d[0];
            reg0.d[0] = input * b1.v.d[0] - a1.v.d[0] * op + reg1.d[0];
            reg1.d[0] = input * b2.v.d[0] - a2.v.d[0] * op;

            dstL[k] = op;

            input = dataR[k];
            op = input * b0.v.d[0] + reg0.d[1];
            reg0.d[1] = input * b1.v.d[0] - a1.v.d[0] * op + reg1.d[1];
            reg1.d[1] = input * b2.v.d[0] - a2.v.d[0] * op;

            dstR[k] = op;
        }
        flush_denormal(reg0.d[0]);
        flush_denormal(reg1.d[0]);
        flush_denormal(reg0.d[1]);
        flush_denormal(reg1.d[1]);
    }
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::process_block(double *data)
{
    {
        int k;
        for (k = 0; k < BLOCK_SIZE; k++)
        {
            a1.process();
            a2.process();
            b0.process();
            b1.process();
            b2.process();

            double input = data[k];
            double op;

            op = input * b0.v.d[0] + reg0.d[0];
            reg0.d[0] = input * b1.v.d[0] - a1.v.d[0] * op + reg1.d[0];
            reg1.d[0] = input * b2.v.d[0] - a2.v.d[0] * op;

            data[k] = op;
        }
        flush_denormal(reg0.d[0]);
        flush_denormal(reg1.d[0]);
    }
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline void BiquadFilter<D, BLOCK_SIZE>::setBlockSize(int bs)
{
    /*	a1.setBlockSize(bs);
            a2.setBlockSize(bs);
            b0.setBlockSize(bs);
            b1.setBlockSize(bs);
            b2.setBlockSize(bs);*/
}

template <typename D, size_t BLOCK_SIZE>
    requires(ValidBiquad<D, BLOCK_SIZE>)
inline float BiquadFilter<D, BLOCK_SIZE>::plot_magnitude(float f)
{
    std::complex<double> ca0(1, 0), ca1(a1.v.d[0], 0), ca2(a2.v.d[0], 0), cb0(b0.v.d[0], 0),
        cb1(b1.v.d[0], 0), cb2(b2.v.d[0], 0);

    std::complex<double> i(0, 1);
    std::complex<double> z = exp(-2 * 3.1415 * f * i);

    std::complex<double> h = (cb0 + cb1 * z + cb2 * z * z) / (ca0 + ca1 * z + ca2 * z * z);

    double r = abs(h);
    return r;
}

} // namespace sst::filters::Biquad

#endif // SURGE_BIQUADFILTER_H
