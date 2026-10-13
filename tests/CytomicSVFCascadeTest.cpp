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

#include "TestUtils.h"
#include "sst/filters/CytomicSVFCascade.h"

#include <cmath>
#include <complex>
#include <random>
#include <vector>

namespace
{
namespace sccd = sst::filters::cytomic_cascade::detail;
using Mode = sst::filters::cytomic_cascade::Mode;
template <Mode M, bool Driven = false, bool Compensated = false>
using Cascade = sst::filters::CytomicSVFCascade<M, Driven, Compensated>;

constexpr double sr = 48000.0;
constexpr float srInv = 1.f / 48000.f;

std::vector<float> testSignal(int len)
{
    std::vector<float> res(len);
    std::minstd_rand gen(8675309);
    std::uniform_real_distribution<float> dist(-1.f, 1.f);
    double saw{0};
    for (int i = 0; i < len; ++i)
    {
        saw += 110.0 / sr;
        if (saw > 1)
            saw -= 2;
        res[i] = (i == 0 ? 1.f : 0.f) + 0.3f * dist(gen) + 0.5f * (float)saw;
    }
    return res;
}

// coefficients for an absolute loop gain r, without the user resonance clamp
void rawCoeffs(double fc, double r, float *c)
{
    auto g = std::tan(M_PI * fc / sr);
    auto q = g / (1 + g);
    c[sccd::Coeff::q] = (float)q;
    c[sccd::Coeff::r] = (float)r;
    c[sccd::Coeff::inv] = (float)(1 / (1 + r * std::pow(q, 4)));
    c[sccd::Coeff::gain] = (float)(1 + r);
}

// the exact response of the linear loop, from the same mixes as step
template <Mode M>
std::complex<double> analytic(double fc, double r, double f, bool compensated = false)
{
    auto g = std::tan(M_PI * fc / sr);
    auto zi = std::exp(std::complex<double>(0, -2.0 * M_PI * f / sr));
    // bilinear 1/(1+s)
    auto t = g * (1. + zi) / (g * (1. + zi) + (1. - zi));
    auto u = 1.0 / (1.0 + r * std::pow(t, 4));
    auto v1a = u * (t - t * t);
    auto v2b = u * std::pow(t, 4);
    auto hp2 = u * t * t * (1. - t) * (1. - t);

    std::complex<double> res;
    switch (M)
    {
    case Mode::Lowpass:
        res = v2b;
        break;
    case Mode::Highpass:
        res = u - 4. * v1a - v2b + 2. * hp2;
        break;
    case Mode::Bandpass:
        res = 4. * hp2;
        break;
    case Mode::Allpass:
        res = 1. - 8. * v1a + 16. * hp2;
        break;
    case Mode::Notch:
    case Mode::Peak:
        res = 1. - (sccd::rMax - r) * (0.5 * hp2 + 0.25 * v1a);
        if (M == Mode::Peak)
            res -= 2 * (1 + r) * v2b;
        break;
    }
    return compensated ? res * (1 + r) : res;
}

template <Mode M, bool Compensated = false> double analyticError(double fc, double res)
{
    float c[sccd::Coeff::count];
    sccd::makeCoeffs<false>(fc, res, srInv, c);
    SIMD_M128 C[sccd::Coeff::count], ic1[2], ic2[2];
    for (int i = 0; i < sccd::Coeff::count; ++i)
        C[i] = SIMD_MM(set1_ps)(c[i]);
    for (int i = 0; i < 2; ++i)
        ic1[i] = ic2[i] = SIMD_MM(setzero_ps)();

    std::vector<double> ir(1 << 16);
    float r4 alignas(16)[4];
    for (size_t i = 0; i < ir.size(); ++i)
    {
        auto o =
            sccd::step<M, false, Compensated>(C, ic1, ic2, SIMD_MM(set1_ps)(i == 0 ? 1.f : 0.f));
        SIMD_MM(store_ps)(r4, o);
        ir[i] = r4[0];
    }

    double maxErr{0}, maxH{0};
    for (int i = 0; i < 60; ++i)
    {
        auto f = 20.0 * std::pow(2.0, i * std::log2(23000.0 / 20.0) / 59);
        auto rot = std::exp(std::complex<double>(0, -2.0 * M_PI * f / sr));
        std::complex<double> zn{1, 0}, dtft{0, 0};
        for (auto v : ir)
        {
            dtft += v * zn;
            zn *= rot;
        }
        auto h = analytic<M>(fc, c[sccd::Coeff::r], f, Compensated);
        maxErr = std::max(maxErr, std::abs(dtft - h));
        maxH = std::max(maxH, std::abs(h));
    }
    return maxErr / maxH;
}

// max |a - b| / max |b| over the test signal
template <typename A, typename B> double relError(A &&a, B &&b, double scale)
{
    auto sig = testSignal(48000);
    double maxErr{0}, maxRef{0};
    for (auto s : sig)
    {
        auto x = (float)(scale * s);
        auto oa = a(x), ob = b(x);
        maxErr = std::max(maxErr, (double)std::fabs(oa - ob));
        maxRef = std::max(maxRef, (double)std::fabs(ob));
    }
    return maxErr / maxRef;
}

// a driven filter fed a tiny signal against its linear twin at the same loop gain
template <Mode M> double drivenSmallSignalError(double fc, double res)
{
    Cascade<M, true> driven;
    Cascade<M, false> linear;
    driven.setCoeff(fc, res / sccd::drivenResonanceMax, srInv);
    linear.setCoeff(fc, res, srInv);
    constexpr float tiny = 1e-3f;
    return relError(
        [&](float x) {
            float o = x * tiny;
            driven.processBlockStep(o);
            return o / tiny;
        },
        [&](float x) {
            linear.processBlockStep(x);
            return x;
        },
        1.0);
}

struct OscResult
{
    double ratio; // peak in 3..4s over peak in 1..2s
    double freq;  // by interpolated zero crossings over the last 2s
};

OscResult selfOscillate(double fc, double r, bool driven = false)
{
    float c[sccd::Coeff::count];
    rawCoeffs(fc, r, c);
    SIMD_M128 C[sccd::Coeff::count], ic1[2], ic2[2];
    for (int i = 0; i < sccd::Coeff::count; ++i)
        C[i] = SIMD_MM(set1_ps)(c[i]);
    for (int i = 0; i < 2; ++i)
        ic1[i] = ic2[i] = SIMD_MM(setzero_ps)();

    int len = (int)(4 * sr);
    std::vector<double> y(len);
    float r4 alignas(16)[4];
    for (int i = 0; i < len; ++i)
    {
        auto in = SIMD_MM(set1_ps)(i == 0 ? 1e-3f : 0.f);
        auto o = driven ? sccd::step<Mode::Lowpass, true, false>(C, ic1, ic2, in)
                        : sccd::step<Mode::Lowpass, false, false>(C, ic1, ic2, in);
        SIMD_MM(store_ps)(r4, o);
        y[i] = r4[0];
    }

    auto peak = [&](double s0, double s1) {
        double p{0};
        for (int i = (int)(s0 * sr); i < (int)(s1 * sr); ++i)
            p = std::max(p, std::fabs(y[i]));
        return p;
    };

    double firstX{-1}, lastX{-1};
    int crossings{0};
    for (int i = (int)(2 * sr); i < len - 1; ++i)
    {
        if (y[i] <= 0 && y[i + 1] > 0)
        {
            auto x = i + y[i] / (y[i] - y[i + 1]);
            if (firstX < 0)
                firstX = x;
            else
                crossings++;
            lastX = x;
        }
    }
    return {peak(3, 4) / peak(1, 2), crossings * sr / (lastX - firstX)};
}

template <typename F> void stressFilter(bool fm, bool sawLfo, double lfoHz, double seconds = 10)
{
    constexpr int bs = 16;
    const int blocks = (int)(seconds * sr / bs);
    F f;
    std::minstd_rand gen(2112);
    std::uniform_real_distribution<float> dist(-1.f, 1.f);

    double saw{0}, lfo{0}, maxOut{0};
    float in[bs], freq[bs], out[bs];
    for (int b = 0; b < blocks; ++b)
    {
        auto res = 0.495f + 0.495f * std::sin(b * 0.001f);
        for (int i = 0; i < bs; ++i)
        {
            saw += 110 / sr;
            if (saw > 1)
                saw -= 2;
            in[i] = b % 2 ? (float)saw : dist(gen);
            lfo += lfoHz / sr;
            if (lfo > 1)
                lfo -= 1;
            auto l = sawLfo ? lfo : 0.5 + 0.5 * std::sin(2 * M_PI * lfo);
            freq[i] = (float)(20.0 * std::pow(0.49 * sr / 20.0, l));
        }
        f.template setCoeffForBlock<bs>(freq[0], res, srInv);
        if (fm)
            f.template processBlockFM<bs>(in, freq, out);
        else
            f.template processBlock<bs>(in, out);
        for (int i = 0; i < bs; ++i)
        {
            REQUIRE(std::isfinite(out[i]));
            maxOut = std::max(maxOut, (double)std::fabs(out[i]));
        }
    }
    INFO("maxOut=" << maxOut);
    REQUIRE(maxOut < 100);
}
} // namespace

TEST_CASE("Cytomic Cascade Analytic Response")
{
    for (auto res : {0.0, 0.5, 0.8, 0.95})
    {
        DYNAMIC_SECTION("res=" << res)
        {
            REQUIRE(analyticError<Mode::Lowpass>(1000, res) < 5e-5);
            REQUIRE(analyticError<Mode::Lowpass, true>(1000, res) < 5e-5);
            REQUIRE(analyticError<Mode::Highpass>(1000, res) < 5e-5);
            REQUIRE(analyticError<Mode::Bandpass>(1000, res) < 5e-5);
            REQUIRE(analyticError<Mode::Notch>(1000, res) < 5e-5);
            REQUIRE(analyticError<Mode::Peak>(1000, res) < 5e-5);
            REQUIRE(analyticError<Mode::Allpass>(1000, res) < 5e-5);
        }
    }
}

TEST_CASE("Cytomic Cascade Response Identities")
{
    for (auto res : {0.0, 0.5, 0.95})
    {
        auto r = res * sccd::rMax;
        INFO("res=" << res);

        // -180 at the cutoff, where the loop resonates
        REQUIRE(std::fabs(std::arg(analytic<Mode::Lowpass>(1000, r, 1000))) ==
                Approx(M_PI).margin(1e-9));
        REQUIRE(std::abs(analytic<Mode::Lowpass>(1000, r, 0, true)) == Approx(1).epsilon(1e-9));
        REQUIRE(std::abs(analytic<Mode::Bandpass>(1000, 0, 1000)) == Approx(1).epsilon(1e-9));

        for (auto f : {0.0, 50.0, 300.0, 1000.0, 2500.0, 12000.0, 23900.0})
            REQUIRE(std::abs(analytic<Mode::Allpass>(1000, r, f)) == Approx(1).epsilon(1e-9));

        // a zero pair on the cutoff, unity at dc and nyquist
        REQUIRE(std::abs(analytic<Mode::Notch>(1000, r, 1000)) < 1e-9);
        REQUIRE(std::abs(analytic<Mode::Notch>(1000, r, 0)) == Approx(1).epsilon(1e-9));
        REQUIRE(std::abs(analytic<Mode::Notch>(1000, r, 24000)) == Approx(1).epsilon(1e-9));

        // dc inverted, nyquist through, a peak at the cutoff which grows with resonance
        REQUIRE(analytic<Mode::Peak>(1000, r, 0).real() == Approx(-1).epsilon(1e-9));
        REQUIRE(analytic<Mode::Peak>(1000, r, 24000).real() == Approx(1).epsilon(1e-9));
        REQUIRE(std::abs(analytic<Mode::Peak>(1000, r, 1000)) ==
                Approx(2 * (1 + r) / (sccd::rMax - r)).epsilon(1e-9));
    }
}

TEST_CASE("Cytomic Cascade Driven Small Signal")
{
    for (auto res : {0.0, 0.5, 0.95})
    {
        INFO("res=" << res);
        REQUIRE(drivenSmallSignalError<Mode::Lowpass>(1000, res) < 1e-3);
        REQUIRE(drivenSmallSignalError<Mode::Highpass>(1000, res) < 1e-3);
        REQUIRE(drivenSmallSignalError<Mode::Bandpass>(1000, res) < 1e-3);
        REQUIRE(drivenSmallSignalError<Mode::Notch>(1000, res) < 1e-3);
        REQUIRE(drivenSmallSignalError<Mode::Peak>(1000, res) < 1e-3);
        REQUIRE(drivenSmallSignalError<Mode::Allpass>(1000, res) < 1e-3);
    }
}

TEST_CASE("Cytomic Cascade Self Oscillation")
{
    for (auto fc : {1000.0, 12000.0})
    {
        auto lo = selfOscillate(fc, 0.999 * sccd::rMax);
        auto hi = selfOscillate(fc, 1.001 * sccd::rMax);
        INFO("fc=" << fc << " ratio lo=" << lo.ratio << " hi=" << hi.ratio << " freq lo=" << lo.freq
                   << " hi=" << hi.freq);
        REQUIRE(lo.ratio < 1);
        REQUIRE(hi.ratio > 1);
        REQUIRE((lo.freq + hi.freq) * 0.5 == Approx(fc).epsilon(5e-4));
    }
}

TEST_CASE("Cytomic Cascade Driven Oscillation")
{
    SECTION("Settles to a bounded limit cycle at the cutoff")
    {
        auto osc = selfOscillate(1000, sccd::drivenResonanceMax * sccd::rMax, true);
        INFO("ratio=" << osc.ratio << " freq=" << osc.freq);
        REQUIRE(osc.ratio == Approx(1).margin(0.01));
        REQUIRE(osc.freq == Approx(1000).epsilon(1e-3));
    }

    SECTION("Compensation leaves the oscillation level alone")
    {
        // the gain sits ahead of the tanh, so it scales the seed but not the limit cycle
        Cascade<Mode::Lowpass, true, true> comp;
        Cascade<Mode::Lowpass, true, false> plain;
        comp.setCoeff(440, 1.0, srInv);
        plain.setCoeff(440, 1.0, srInv);
        float peakC{0}, peakP{0};
        for (int i = 0; i < 2 * 48000; ++i)
        {
            float c = i == 0 ? 1e-3f : 0.f, p = c;
            comp.processBlockStep(c);
            plain.processBlockStep(p);
            if (i > 48000)
            {
                peakC = std::max(peakC, std::fabs(c));
                peakP = std::max(peakP, std::fabs(p));
            }
        }
        REQUIRE(peakP > 0.5f);
        REQUIRE(peakC == Approx(peakP).epsilon(1e-3));
    }
}

TEST_CASE("Cytomic Cascade Lanes")
{
    auto sig = testSignal(4096);
    Cascade<Mode::Lowpass> mono, stereo, right;
    mono.setCoeff(1200, 0.7, srInv);
    stereo.setCoeff(1200, 300, 0.7, 0.2, srInv);
    right.setCoeff(300, 0.2, srInv);
    for (auto s : sig)
    {
        float m = s, l = s, r = s, rr = s;
        mono.processBlockStep(m);
        stereo.processBlockStep(l, r);
        right.processBlockStep(rr);
        REQUIRE(l == m);
        REQUIRE(r == rr);
    }
}

TEST_CASE("Cytomic Cascade FM")
{
    // a constant fm frequency matches block coefficients, up to fast tan
    constexpr int bs = 32;
    auto sig = testSignal(bs * 1000);
    Cascade<Mode::Highpass> block, fm;
    block.setCoeffForBlock<bs>(2000, 0.8, srInv);
    fm.setCoeffForBlock<bs>(2000, 0.8, srInv);

    float freq[bs];
    std::fill(freq, freq + bs, 2000.f);
    double maxErr{0}, maxV{0};
    for (int b = 0; b < 1000; ++b)
    {
        float ob[bs], of[bs];
        block.processBlock<bs>(sig.data() + b * bs, ob);
        fm.processBlockFM<bs>(sig.data() + b * bs, freq, of);
        block.retainCoeffForBlock<bs>();
        fm.retainCoeffForBlock<bs>();
        for (int i = 0; i < bs; ++i)
        {
            maxErr = std::max(maxErr, (double)std::fabs(ob[i] - of[i]));
            maxV = std::max(maxV, (double)std::fabs(ob[i]));
        }
    }
    REQUIRE(maxErr / maxV < 1e-3);
}

TEST_CASE("Cytomic Cascade Cutoff Jumps")
{
    // a saw lfo snaps 23k to 20hz inside one block ramp; the mixes need exact cores there
    stressFilter<Cascade<Mode::Highpass>>(false, true, 5, 1);
    stressFilter<Cascade<Mode::Notch>>(false, true, 5, 1);
    stressFilter<Cascade<Mode::Allpass>>(false, true, 5, 1);
}

TEST_CASE("Cytomic Cascade Modulation Stress", "[.stress]")
{
    for (auto fm : {false, true})
    {
        for (auto saw : {false, true})
        {
            for (auto hz : {1000.0, 5000.0})
            {
                INFO("fm=" << fm << " saw=" << saw << " hz=" << hz);
                stressFilter<Cascade<Mode::Lowpass>>(fm, saw, hz);
                stressFilter<Cascade<Mode::Lowpass, true, true>>(fm, saw, hz);
                stressFilter<Cascade<Mode::Highpass>>(fm, saw, hz);
                stressFilter<Cascade<Mode::Bandpass>>(fm, saw, hz);
                stressFilter<Cascade<Mode::Notch>>(fm, saw, hz);
                stressFilter<Cascade<Mode::Peak, true>>(fm, saw, hz);
                stressFilter<Cascade<Mode::Allpass>>(fm, saw, hz);
            }
        }
    }
}

namespace
{
template <Mode M, bool Driven, bool Compensated>
void quadMatchesStandalone(sst::filters::FilterType type, int subtype)
{
    using tp_t = sst::filters::detail::BasicTuningProvider;
    constexpr float pitch = 7.f, res = 0.7f;

    auto qfu = sst::filters::QuadFilterUnitState{};
    for (int i = 0; i < 4; ++i)
        qfu.active[i] = (int)0xffffffff;
    std::fill(qfu.R, qfu.R + sst::filters::n_filter_registers, SIMD_MM(setzero_ps)());

    sst::filters::FilterCoefficientMaker<tp_t> cm;
    cm.setSampleRateAndBlockSize(sr, 32);
    cm.MakeCoeffs(pitch, res, type, (sst::filters::FilterSubType)subtype, nullptr, false);
    cm.updateState(qfu);
    auto fn = sst::filters::GetQFPtrFilterUnit(type, (sst::filters::FilterSubType)subtype);
    REQUIRE(fn);

    Cascade<M, Driven, Compensated> sa;
    sa.setCoeff(440.f * tp_t::note_to_pitch_ignoring_tuning(pitch), res, srInv);

    float r4 alignas(16)[4];
    for (auto s : testSignal(4096))
    {
        SIMD_MM(store_ps)(r4, fn(&qfu, SIMD_MM(set1_ps)(s)));
        float o = s;
        sa.processBlockStep(o);
        REQUIRE(r4[0] == o);
        REQUIRE(r4[3] == o);
    }
}

template <Mode M> void quadMatchesStandaloneAll(sst::filters::FilterType type)
{
    quadMatchesStandalone<M, false, false>(type, 0);
    quadMatchesStandalone<M, true, false>(type, 1);
    if constexpr (M == Mode::Lowpass)
    {
        quadMatchesStandalone<M, false, true>(type, 2);
        quadMatchesStandalone<M, true, true>(type, 3);
    }
}
} // namespace

TEST_CASE("Cytomic Cascade Quad Form")
{
    using namespace sst::filters;

    SECTION("Quad form matches standalone for every subtype")
    {
        quadMatchesStandaloneAll<Mode::Lowpass>(fut_cytomic_cascade_lp);
        quadMatchesStandaloneAll<Mode::Highpass>(fut_cytomic_cascade_hp);
        quadMatchesStandaloneAll<Mode::Bandpass>(fut_cytomic_cascade_bp);
        quadMatchesStandaloneAll<Mode::Notch>(fut_cytomic_cascade_n);
        quadMatchesStandaloneAll<Mode::Peak>(fut_cytomic_cascade_pk);
        quadMatchesStandaloneAll<Mode::Allpass>(fut_cytomic_cascade_ap);
    }

    SECTION("Subtype bounds")
    {
        REQUIRE(fut_subcount[fut_cytomic_cascade_lp] == 4);
        REQUIRE(fut_subcount[fut_cytomic_cascade_hp] == 2);
        REQUIRE(GetQFPtrFilterUnit(fut_cytomic_cascade_lp, st_cascade_driven_compensated));
        REQUIRE_FALSE(GetQFPtrFilterUnit(fut_cytomic_cascade_hp, st_cascade_compensated));
        REQUIRE_FALSE(GetQFPtrFilterUnit(fut_cytomic_cascade_ap, st_cascade_compensated));
    }

    SECTION("Filters++ configurations all resolve")
    {
        namespace sfpp = sst::filtersplusplus;
        auto configs =
            sfpp::Filter::availableModelConfigurations(sfpp::FilterModel::CytomicCascade);
        REQUIRE(configs.size() == 14);
        for (const auto &c : configs)
        {
            auto filter = sfpp::Filter();
            filter.setFilterModel(sfpp::FilterModel::CytomicCascade);
            filter.setModelConfiguration(c);
            filter.setSampleRateAndBlockSize(48000, 16);
            REQUIRE(filter.prepareInstance());
        }
    }
}

TEST_CASE("Cytomic Cascade RMS")
{
    using namespace TestUtils;
    namespace sfpp = sst::filtersplusplus;
    using sfpp::DriveMode, sfpp::Passband, sfpp::Slope, sfpp::FilterSubModel;

    auto check = [](sst::filters::FilterType ft, sst::filters::FilterSubType st, Passband pb,
                    DriveMode dm, FilterSubModel sm, const RMSSet &r) {
        runTest({ft, st, r});
        runTest(sfpp::FilterModel::CytomicCascade, {pb, Slope::Slope_24dB, dm, sm}, 0.f, 0.5f, r);
    };
    auto none = FilterSubModel::UNSUPPORTED;

    SECTION("LP")
    {
        check(FilterType::fut_cytomic_cascade_lp, FilterSubType::st_cascade_standard, Passband::LP,
              DriveMode::Standard, none, {-12.1765f, -10.3101f, -9.09885f, -32.45f, -59.0416f});
    }
    SECTION("LP Driven Compensated")
    {
        check(FilterType::fut_cytomic_cascade_lp, FilterSubType::st_cascade_driven_compensated,
              Passband::LP, DriveMode::Driven, FilterSubModel::Compensated,
              {-0.688022f, 2.02021f, 0.504791f, -21.3188f, -47.5792f});
    }
    SECTION("HP")
    {
        check(FilterType::fut_cytomic_cascade_hp, FilterSubType::st_cascade_standard, Passband::HP,
              DriveMode::Standard, none, {-47.2411f, -34.4863f, -9.22529f, -6.07377f, -3.03727f});
    }
    SECTION("BP Driven")
    {
        check(FilterType::fut_cytomic_cascade_bp, FilterSubType::st_cascade_driven, Passband::BP,
              DriveMode::Driven, none, {-28.7136f, -12.7068f, 4.23534f, -8.21649f, -44.9675f});
    }
    SECTION("Notch")
    {
        check(FilterType::fut_cytomic_cascade_n, FilterSubType::st_cascade_standard,
              Passband::Notch, DriveMode::Standard, none,
              {-2.79602f, -2.19024f, -19.2834f, -3.87713f, -3.01084f});
    }
    SECTION("Peak Driven")
    {
        check(FilterType::fut_cytomic_cascade_pk, FilterSubType::st_cascade_driven, Passband::Peak,
              DriveMode::Driven, none, {-1.903f, 1.93462f, 9.4368f, -3.76285f, -3.01116f});
    }
    SECTION("Allpass")
    {
        check(FilterType::fut_cytomic_cascade_ap, FilterSubType::st_cascade_standard,
              Passband::Allpass, DriveMode::Standard, none,
              {-3.02553f, -3.12172f, -3.31199f, -3.06092f, -3.0131f});
    }
}
