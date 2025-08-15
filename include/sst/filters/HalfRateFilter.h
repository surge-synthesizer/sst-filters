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
#ifndef INCLUDE_SST_FILTERS_HALFRATEFILTER_H
#define INCLUDE_SST_FILTERS_HALFRATEFILTER_H

#include <cstdint>
#include <cmath>
#include "sst/utilities/globals.h"

namespace sst::filters::HalfRate
{

static constexpr uint32_t halfrate_max_M = 6;
static constexpr uint32_t hr_BLOCK_SIZE = 256;

class alignas(16) HalfRateFilter
{
  private:
    // Remember leave these first so they stay aligned
    SIMD_M128 va[halfrate_max_M];
    SIMD_M128 vx0[halfrate_max_M];
    SIMD_M128 vx1[halfrate_max_M];
    SIMD_M128 vx2[halfrate_max_M];
    SIMD_M128 vy0[halfrate_max_M];
    SIMD_M128 vy1[halfrate_max_M];
    SIMD_M128 vy2[halfrate_max_M];
    SIMD_M128 oldout;

    const SIMD_M128 half = SIMD_MM(set_ps1)(0.5f);

  public:
    /**
     * Implement a half rate up/down filter
     *
     * @param M The size of the FIR. Range from 1 to halfrate_max_M
     * @param steep Steepness. Set false for softer slopes, more attenuation and less stopband
     * ripple
     */
    HalfRateFilter(uint32_t M, bool steep)
    {
        assert(!(M > halfrate_max_M));
        this->M = M;
        this->steep = steep;
        load_coefficients();
        reset();
    }

    void process_block(float *floatL, float *floatR, int nsamples)
    {
        SIMD_M128 *__restrict L = (SIMD_M128 *)floatL;
        SIMD_M128 *__restrict R = (SIMD_M128 *)floatR;
        SIMD_M128 o[hr_BLOCK_SIZE];
        auto N = nsamples;
        // fill the buffer with interleaved stereo samples
        for (int k = 0; k < N; k += 4)
        {
            //[o3,o2,o1,o0] = [L0,L0,R0,R0]
            o[k] = SIMD_MM(shuffle_ps)(L[k >> 2], R[k >> 2], SIMD_MM_SHUFFLE(0, 0, 0, 0));
            o[k + 1] = SIMD_MM(shuffle_ps)(L[k >> 2], R[k >> 2], SIMD_MM_SHUFFLE(1, 1, 1, 1));
            o[k + 2] = SIMD_MM(shuffle_ps)(L[k >> 2], R[k >> 2], SIMD_MM_SHUFFLE(2, 2, 2, 2));
            o[k + 3] = SIMD_MM(shuffle_ps)(L[k >> 2], R[k >> 2], SIMD_MM_SHUFFLE(3, 3, 3, 3));
        }

        // process filters
        for (auto j = 0U; j < M; j++)
        {
            auto tx0 = vx0[j];
            auto tx1 = vx1[j];
            auto tx2 = vx2[j];
            auto ty0 = vy0[j];
            auto ty1 = vy1[j];
            auto ty2 = vy2[j];
            auto ta = va[j];

            for (int k = 0; k < N; k += 2)
            {
                // shuffle inputs
                tx2 = tx1;
                tx1 = tx0;
                tx0 = o[k];
                // shuffle outputs
                ty2 = ty1;
                ty1 = ty0;
                // allpass filter 1
                ty0 = SIMD_MM(add_ps)(tx2, SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(tx0, ty2), ta));
                o[k] = ty0;

                // shuffle inputs
                tx2 = tx1;
                tx1 = tx0;
                tx0 = o[k + 1];
                // shuffle outputs
                ty2 = ty1;
                ty1 = ty0;
                // allpass filter 1
                ty0 = SIMD_MM(add_ps)(tx2, SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(tx0, ty2), ta));
                o[k + 1] = ty0;
            }
            vx0[j] = tx0;
            vx1[j] = tx1;
            vx2[j] = tx2;
            vy0[j] = ty0;
            vy1[j] = ty1;
            vy2[j] = ty2;
        }

        /*for(int k=0; k<nsamples; k++)
        {
        float *of = (float*)o;
        float out_a = of[(k<<2)];
        float out_b = of[(k<<2)+1];

        float *fL = (float*)L;
        fL[k] = (out_a + oldout_f) * 0.5f;
        oldout_f = out_b;

        out_a = of[(k<<2)+2];
        out_b = of[(k<<2)+3];

        float *fR = (float*)R;
        fR[k] = (out_a + oldoutR_f) * 0.5f;
        oldoutR_f = out_b;
        }*/

        float *fL = (float *)L;
        float *fR = (float *)R;
        auto faR = SIMD_MM(setzero_ps)();
        auto fbR = SIMD_MM(setzero_ps)();

        for (int k = 0; k < N; k++)
        {
            //	const double output=(filter_a.process(input)+oldout)*0.5;
            //	oldout=filter_b.process(input);

            auto vL = SIMD_MM(add_ss)(o[k], oldout);
            vL = SIMD_MM(mul_ss)(vL, half);
            SIMD_MM(store_ss)(&fL[k], vL);

            faR = SIMD_MM(movehl_ps)(faR, o[k]);
            fbR = SIMD_MM(movehl_ps)(fbR, oldout);

            auto vR = SIMD_MM(add_ss)(faR, fbR);
            vR = SIMD_MM(mul_ss)(vR, half);
            SIMD_MM(store_ss)(&fR[k], vR);

            oldout = SIMD_MM(shuffle_ps)(o[k], o[k], SIMD_MM_SHUFFLE(3, 3, 1, 1));
        }
    }
    void process_block_D2(float *floatL, float *floatR, int nsamples, float *outL = 0,
                          float *outR = 0) // process in-place. the new block will be half the size
    {
        auto *L = (SIMD_M128 *)floatL;
        auto *R = (SIMD_M128 *)floatR;
        SIMD_M128 o[hr_BLOCK_SIZE];

        /*
         * fill the buffer with interleaved stereo samples by rotating the
         * input simd-in-time a bit
         *
         * SIMD_MM(shuffle_ps)(a,b,SIMD_MM_SHUFFLE(i,j,k,l)) returns a[i], a[j], b[k], b[l]
         *
         * So this loop makes o look like the rotation of L and R. That is
         *
         * o[n] = {L[n],L[n],R[n],R[n]}
         *
         * for n in 0..nsamples leading o to being LLRR pairs of the input
         */
        for (int k = 0; k < nsamples; k += 4)
        {
            o[k] = SIMD_MM(shuffle_ps)(L[k >> 2], R[k >> 2], SIMD_MM_SHUFFLE(0, 0, 0, 0));
            o[k + 1] = SIMD_MM(shuffle_ps)(L[k >> 2], R[k >> 2], SIMD_MM_SHUFFLE(1, 1, 1, 1));
            o[k + 2] = SIMD_MM(shuffle_ps)(L[k >> 2], R[k >> 2], SIMD_MM_SHUFFLE(2, 2, 2, 2));
            o[k + 3] = SIMD_MM(shuffle_ps)(L[k >> 2], R[k >> 2], SIMD_MM_SHUFFLE(3, 3, 3, 3));
        }

        /*
         * Process filters. We have a cascade of M three point filters we need to run.
         * Each has a state vx0..2 and vy0..2 indexed by the filter M. We also have coefficients
         * for each filter A_M and B_M. which are loaded into ta in set_coefficients in order
         * B A B A in SIMD space
         *
         * The filter step is x2 = x1; x1 = x0; x0 = o. Which means the X have the LL RR form.
         *
         * Then y2 = y1, y1 = y0 and y0 = x2 + (x0-y2) * a
         *
         * Then o = y0. Fine. But what's that filter step doing. Basically it is saying for each
         * L and R pair we get two AllPass updates. So expanding the vector you get
         *
         * 0: y0_0 = x2_0 + (L - y2_0) + B
         * 1: y0_0 = x2_0 + (L - y2_0) + A
         *
         * etc... - basically we have done a pair of all passes at each M.
         *
         * So at the end of this loop for each order, the 'o' array will be filled
         * with the cascade of coefficient B or A all passes and will be in order
         *
         * o_i: AllPassCascade_B(L_i), AllPassCascade_A(L_i), AllPassCascade_B(R_i),
         * AllPassCascade_A(R_i)
         */
        for (auto j = 0U; j < M; j++)
        {
            auto tx0 = vx0[j];
            auto tx1 = vx1[j];
            auto tx2 = vx2[j];
            auto ty0 = vy0[j];
            auto ty1 = vy1[j];
            auto ty2 = vy2[j];
            auto ta = va[j];

            // Why is this loop hand-unrolled?
            for (int k = 0; k < nsamples; k += 2)
            {
                // shuffle inputs
                tx2 = tx1;
                tx1 = tx0;
                tx0 = o[k];
                // shuffle outputs
                ty2 = ty1;
                ty1 = ty0;
                // allpass filter 1
                ty0 = SIMD_MM(add_ps)(tx2, SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(tx0, ty2), ta));
                o[k] = ty0;

                // shuffle inputs
                tx2 = tx1;
                tx1 = tx0;
                tx0 = o[k + 1];
                // shuffle outputs
                ty2 = ty1;
                ty1 = ty0;
                // allpass filter 1
                ty0 = SIMD_MM(add_ps)(tx2, SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(tx0, ty2), ta));
                o[k + 1] = ty0;
            }
            vx0[j] = tx0;
            vx1[j] = tx1;
            vx2[j] = tx2;
            vy0[j] = ty0;
            vy1[j] = ty1;
            vy2[j] = ty2;
        }

        auto aR = SIMD_MM(setzero_ps)();
        auto bR = SIMD_MM(setzero_ps)();
        auto cR = SIMD_MM(setzero_ps)();
        auto dR = SIMD_MM(setzero_ps)();

        if (outL)
            L = (SIMD_M128 *)outL;
        if (outR)
            R = (SIMD_M128 *)outR;

        /*
         * OK so now we have all the filtered signals we want to reconstruct the output.
         * This is basically the sample selection stage. To read this code you need
         * to remember that SIMD_MM(movehl_ps)(a,b) results in b[3], b[4], a[3], a[4] as the
         * simd output.
         *
         * The code had this comment
         *
         *  const double output=(filter_a.process(input)+oldout)*0.5;
         *  oldout=filter_b.process(input);
         *
         * atop this code
         *
         * auto tL0 = SIMD_MM(shuffle_ps)(o[k], o[k], SIMD_MM_SHUFFLE(1, 1, 1, 1));
         * auto tR0 = SIMD_MM(shuffle_ps)(o[k], o[k], SIMD_MM_SHUFFLE(3, 3, 3, 3));
         * auto aL = SIMD_MM(add_ss)(tL0, o[k + 1]);
         * aR = SIMD_MM(movehl_ps)(aR, o[k + 1]);
         * aR = SIMD_MM(add_ss)(aR, tR0);
         *
         * So can we make that tie out? Remembering o now has the for B_L A_L, B_R, A_R
         *
         * So tL0 = SIMD_MM(shuffle_ps)(o[k], o[k], 11111)
         * or tL0 = o[k][1] in every slot or tl0 is A_L across the board at sample k.
         * Similarly tR0 = A_R across the board at sample K.
         *
         * Now recall SIMD_MM(add_ss)(a,b) gives you (a[0]+b[0], a[1], a[2], a[3]) so now we do
         *
         * auto aL = SIMD_MM(add_ss)(tL0, o[k + 1]);
         * aL = (A_L[k] + B_L[k+1], A_L[k],  A_L[k], A_L[k]);
         *
         * Now
         *
         * aR = SIMD_MM(movehl_ps)(aR, o[k + 1]);
         * aR = B_R[k+1], A_R[k+1], aR[3], aR[4]
         *
         * aR = SIMD_MM(add_ss)(aR, tR0) or
         * aR = (A_R[k] + B_R[k+1], A_R[k+1], aR[3], aR[4])
         *
         * (At this point I'm suspecting that the rest of the SIMD registeres in A wont matter)
         *
         * Now similarly bL and bR, cL and cR, dL and dR are constructed with the same code just
         * at index k+2/3, k+4/5, k+6/7
         *
         * So once those stages are assembled we do this
         *
         * aL = SIMD_MM(movelh_ps)(aL, bL);
         * cL = SIMD_MM(movelh_ps)(cL, dL);
         * L[k >> 3] = SIMD_MM(shuffle_ps)(aL, cL, SIMD_MM_SHUFFLE(2, 0, 2, 0));
         *
         * And similarly for R. So what's that doing. So first of all _mm_novelh_ps [note lh
         * not hl] has signatlre
         *
         * SIMD_MM(movelh_ps)(a,b) = a[0],a[1],b[0],b[1]
         *
         * so this sets
         *
         * aL = aL[0],aL[1],bL[0],bL[1]
         * cL = cL[0], cL[1], dL[0], dL[1]
         *
         * and then that shuffle says
         *
         * L = aL[2],aL[0],cL[2],cL[0]
         *
         * which looks a lot to me like I have a bit flip somewhere wrong in my
         * comments (like that should be 0202 not 2020 but leave that for now)
         * because essentially all this complexity is resulting in
         *
         * L[i] = A_L[2*i] + B_L[2+i+1]
         *
         * Namely all this code does two things
         *
         * 1. Run a cascade of all pass filters
         * 2. Interleave the output with offset samples for the downsample between
         *    the two filter sides
         *
         */
        for (int k = 0; k < nsamples; k += 8)
        {
            /*	const double output=(filter_a.process(input)+oldout)*0.5;
            oldout=filter_b.process(input);*/

            auto tL0 = SIMD_MM(shuffle_ps)(o[k], o[k], SIMD_MM_SHUFFLE(1, 1, 1, 1));
            auto tR0 = SIMD_MM(shuffle_ps)(o[k], o[k], SIMD_MM_SHUFFLE(3, 3, 3, 3));
            auto aL = SIMD_MM(add_ss)(tL0, o[k + 1]);
            aR = SIMD_MM(movehl_ps)(aR, o[k + 1]);
            aR = SIMD_MM(add_ss)(aR, tR0);

            tL0 = SIMD_MM(shuffle_ps)(o[k + 2], o[k + 2], SIMD_MM_SHUFFLE(1, 1, 1, 1));
            tR0 = SIMD_MM(shuffle_ps)(o[k + 2], o[k + 2], SIMD_MM_SHUFFLE(3, 3, 3, 3));
            auto bL = SIMD_MM(add_ss)(tL0, o[k + 3]);
            bR = SIMD_MM(movehl_ps)(aR, o[k + 3]);
            bR = SIMD_MM(add_ss)(bR, tR0);

            tL0 = SIMD_MM(shuffle_ps)(o[k + 4], o[k + 4], SIMD_MM_SHUFFLE(1, 1, 1, 1));
            tR0 = SIMD_MM(shuffle_ps)(o[k + 4], o[k + 4], SIMD_MM_SHUFFLE(3, 3, 3, 3));
            auto cL = SIMD_MM(add_ss)(tL0, o[k + 5]);
            cR = SIMD_MM(movehl_ps)(cR, o[k + 5]);
            cR = SIMD_MM(add_ss)(cR, tR0);

            tL0 = SIMD_MM(shuffle_ps)(o[k + 6], o[k + 6], SIMD_MM_SHUFFLE(1, 1, 1, 1));
            tR0 = SIMD_MM(shuffle_ps)(o[k + 6], o[k + 6], SIMD_MM_SHUFFLE(3, 3, 3, 3));
            auto dL = SIMD_MM(add_ss)(tL0, o[k + 7]);
            dR = SIMD_MM(movehl_ps)(dR, o[k + 7]);
            dR = SIMD_MM(add_ss)(dR, tR0);

            aL = SIMD_MM(movelh_ps)(aL, bL);
            cL = SIMD_MM(movelh_ps)(cL, dL);
            aR = SIMD_MM(movelh_ps)(aR, bR);
            cR = SIMD_MM(movelh_ps)(cR, dR);

            L[k >> 3] = SIMD_MM(shuffle_ps)(aL, cL, SIMD_MM_SHUFFLE(2, 0, 2, 0));
            R[k >> 3] = SIMD_MM(shuffle_ps)(aR, cR, SIMD_MM_SHUFFLE(2, 0, 2, 0));

            // optional: *=0.5;
            const auto half = SIMD_MM(set_ps1)(0.5f);
            L[k >> 3] = SIMD_MM(mul_ps)(L[k >> 3], half);
            R[k >> 3] = SIMD_MM(mul_ps)(R[k >> 3], half);
        }
    }

    void process_block_U2(float *floatL_in, float *floatR_in, float *floatL, float *floatR,
                          int nsamples)
    {
        auto *L = (SIMD_M128 *)floatL;
        auto *R = (SIMD_M128 *)floatR;
        auto *L_in = (SIMD_M128 *)floatL_in;
        auto *R_in = (SIMD_M128 *)floatR_in;

        SIMD_M128 o[hr_BLOCK_SIZE];
        // fill the buffer with interleaved stereo samples
        for (int k = 0; k < nsamples; k += 8)
        {
            //[o3,o2,o1,o0] = [L0,L0,R0,R0]
            o[k] = SIMD_MM(shuffle_ps)(L_in[k >> 3], R_in[k >> 3], SIMD_MM_SHUFFLE(0, 0, 0, 0));
            o[k + 1] = SIMD_MM(setzero_ps)();
            o[k + 2] = SIMD_MM(shuffle_ps)(L_in[k >> 3], R_in[k >> 3], SIMD_MM_SHUFFLE(1, 1, 1, 1));
            o[k + 3] = SIMD_MM(setzero_ps)();
            o[k + 4] = SIMD_MM(shuffle_ps)(L_in[k >> 3], R_in[k >> 3], SIMD_MM_SHUFFLE(2, 2, 2, 2));
            o[k + 5] = SIMD_MM(setzero_ps)();
            o[k + 6] = SIMD_MM(shuffle_ps)(L_in[k >> 3], R_in[k >> 3], SIMD_MM_SHUFFLE(3, 3, 3, 3));
            o[k + 7] = SIMD_MM(setzero_ps)();
        }

        // process filters
        for (auto j = 0U; j < M; j++)
        {
            auto tx0 = vx0[j];
            auto tx1 = vx1[j];
            auto tx2 = vx2[j];
            auto ty0 = vy0[j];
            auto ty1 = vy1[j];
            auto ty2 = vy2[j];
            auto ta = va[j];

            for (int k = 0; k < nsamples; k += 2)
            {
                // shuffle inputs
                tx2 = tx1;
                tx1 = tx0;
                tx0 = o[k];
                // shuffle outputs
                ty2 = ty1;
                ty1 = ty0;
                // allpass filter 1
                ty0 = SIMD_MM(add_ps)(tx2, SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(tx0, ty2), ta));
                o[k] = ty0;

                // shuffle inputs
                tx2 = tx1;
                tx1 = tx0;
                tx0 = o[k + 1];
                // shuffle outputs
                ty2 = ty1;
                ty1 = ty0;
                // allpass filter 1
                ty0 = SIMD_MM(add_ps)(tx2, SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(tx0, ty2), ta));
                o[k + 1] = ty0;
            }
            vx0[j] = tx0;
            vx1[j] = tx1;
            vx2[j] = tx2;
            vy0[j] = ty0;
            vy1[j] = ty1;
            vy2[j] = ty2;
        }

        /*auto aR = SIMD_MM(setzero_ps)();
        auto bR = SIMD_MM(setzero_ps)();
        auto cR = SIMD_MM(setzero_ps)();
        auto dR = SIMD_MM(setzero_ps)();*/

        float *fL = (float *)L;
        float *fR = (float *)R;
        auto faR = SIMD_MM(setzero_ps)();
        auto fbR = SIMD_MM(setzero_ps)();

        for (int k = 0; k < nsamples; k++)
        {
            //	const double output=(filter_a.process(input)+oldout)*0.5;
            //	oldout=filter_b.process(input);

            auto vL = SIMD_MM(add_ss)(o[k], oldout);
            vL = SIMD_MM(mul_ss)(vL, half);
            SIMD_MM(store_ss)(&fL[k], vL);

            faR = SIMD_MM(movehl_ps)(faR, o[k]);
            fbR = SIMD_MM(movehl_ps)(fbR, oldout);

            auto vR = SIMD_MM(add_ss)(faR, fbR);
            vR = SIMD_MM(mul_ss)(vR, half);
            SIMD_MM(store_ss)(&fR[k], vR);

            oldout = SIMD_MM(shuffle_ps)(o[k], o[k], SIMD_MM_SHUFFLE(3, 3, 1, 1));
        }

        // If you want to avoid downsampling, do this
        /*float oldout_f = 0.f;
        for(unsigned int k=0; k<nsamples; k++)
        {

        float *of = (float*)o;
        float out_a = of[(k<<2)];
        float out_b = of[(k<<2)+1];


        float *fL = (float*)L;
        fL[k] = (out_a + oldout_f) * 0.5f;
        oldout_f = out_b;
        }		*/
    }

    void load_coefficients()
    {
        for (auto i = 0U; i < M; i++)
        {
            va[i] = SIMD_MM(setzero_ps)();
        }

        int order = M << 1;
        if (steep)
        {
            if (order == 12) // rejection=104dB, transition band=0.01
            {
                float a_coefficients[6] = {0.036681502163648017f, 0.2746317593794541f,
                                           0.56109896978791948f,  0.769741833862266f,
                                           0.8922608180038789f,   0.962094548378084f};

                float b_coefficients[6] = {0.13654762463195771f, 0.42313861743656667f,
                                           0.6775400499741616f,  0.839889624849638f,
                                           0.9315419599631839f,  0.9878163707328971f};

                set_coefficients(a_coefficients, b_coefficients);
            }
            else if (order == 10) // rejection=86dB, transition band=0.01
            {
                float a_coefficients[5] = {0.051457617441190984f, 0.35978656070567017f,
                                           0.6725475931034693f, 0.8590884928249939f,
                                           0.9540209867860787f};

                float b_coefficients[5] = {0.18621906251989334f, 0.529951372847964f,
                                           0.7810257527489514f, 0.9141815687605308f,
                                           0.985475023014907f};

                set_coefficients(a_coefficients, b_coefficients);
            }
            else if (order == 8) // rejection=69dB, transition band=0.01
            {
                float a_coefficients[4] = {0.07711507983241622f, 0.4820706250610472f,
                                           0.7968204713315797f, 0.9412514277740471f};

                float b_coefficients[4] = {0.2659685265210946f, 0.6651041532634957f,
                                           0.8841015085506159f, 0.9820054141886075f};

                set_coefficients(a_coefficients, b_coefficients);
            }
            else if (order == 6) // rejection=51dB, transition band=0.01
            {
                float a_coefficients[3] = {0.1271414136264853f, 0.6528245886369117f,
                                           0.9176942834328115f};

                float b_coefficients[3] = {0.40056789819445626f, 0.8204163891923343f,
                                           0.9763114515836773f};

                set_coefficients(a_coefficients, b_coefficients);
            }
            else if (order == 4) // rejection=53dB,transition band=0.05
            {
                float a_coefficients[2] = {0.12073211751675449f, 0.6632020224193995f};

                float b_coefficients[2] = {0.3903621872345006f, 0.890786832653497f};

                set_coefficients(a_coefficients, b_coefficients);
            }

            else // order=2, rejection=36dB, transition band=0.1
            {
                float a_coefficients = 0.23647102099689224f;
                float b_coefficients = 0.7145421497126001f;

                set_coefficients(&a_coefficients, &b_coefficients);
            }
        }
        else // softer slopes, more attenuation and less stopband ripple
        {
            if (order == 12) // rejection=150dB, transition band=0.05
            {
                float a_coefficients[6] = {0.01677466677723562f, 0.13902148819717805f,
                                           0.3325011117394731f,  0.53766105314488f,
                                           0.7214184024215805f,  0.8821858402078155f};

                float b_coefficients[6] = {0.06501319274445962f, 0.23094129990840923f,
                                           0.4364942348420355f,  0.6329609551399348f,
                                           0.80378086794111226f, 0.9599687404800694f};

                set_coefficients(a_coefficients, b_coefficients);
            }
            else if (order == 10) // rejection=133dB, transition band=0.05
            {
                float a_coefficients[5] = {0.02366831419883467f, 0.18989476227180174f,
                                           0.43157318062118555f, 0.6632020224193995f,
                                           0.860015542499582f};

                float b_coefficients[5] = {0.09056555904993387f, 0.3078575723749043f,
                                           0.5516782402507934f, 0.7652146863779808f,
                                           0.95247728378667541f};

                set_coefficients(a_coefficients, b_coefficients);
            }
            else if (order == 8) // rejection=106dB, transition band=0.05
            {
                float a_coefficients[4] = {0.03583278843106211f, 0.2720401433964576f,
                                           0.5720571972357003f, 0.827124761997324f};

                float b_coefficients[4] = {0.1340901419430669f, 0.4243248712718685f,
                                           0.7062921421386394f, 0.9415030941737551f};

                set_coefficients(a_coefficients, b_coefficients);
            }
            else if (order == 6) // rejection=80dB, transition band=0.05
            {
                float a_coefficients[3] = {0.06029739095712437f, 0.4125907203610563f,
                                           0.7727156537429234f};

                float b_coefficients[3] = {0.21597144456092948f, 0.6043586264658363f,
                                           0.9238861386532906f};

                set_coefficients(a_coefficients, b_coefficients);
            }
            else if (order == 4) // rejection=70dB,transition band=0.1
            {
                float a_coefficients[2] = {0.07986642623635751f, 0.5453536510711322f};

                float b_coefficients[2] = {0.28382934487410993f, 0.8344118914807379f};

                set_coefficients(a_coefficients, b_coefficients);
            }

            else // order=2, rejection=36dB, transition band=0.1
            {
                float a_coefficients = 0.23647102099689224f;
                float b_coefficients = 0.7145421497126001f;

                set_coefficients(&a_coefficients, &b_coefficients);
            }
        }
    }
    void set_coefficients(float *cA, float *cB)
    {
        for (auto i = 0U; i < M; i++)
        {
            // va[i] = SIMD_MM(set_ps)(cA[i],cB[i],cA[i],cB[i]);
            va[i] = SIMD_MM(set_ps)(cB[i], cA[i], cB[i], cA[i]);
        }
    }
    void reset()
    {
        for (auto i = 0U; i < M; i++)
        {
            vx0[i] = SIMD_MM(setzero_ps)();
            vx1[i] = SIMD_MM(setzero_ps)();
            vx2[i] = SIMD_MM(setzero_ps)();
            vy0[i] = SIMD_MM(setzero_ps)();
            vy1[i] = SIMD_MM(setzero_ps)();
            vy2[i] = SIMD_MM(setzero_ps)();
        }
        oldout = SIMD_MM(setzero_ps)();
    }

  private:
    uint32_t M;
    bool steep;
    // unsigned int BLOCK_SIZE;
};

} // namespace sst::filters::HalfRate

#endif // SST_FILTERS_K35FILTER_H
