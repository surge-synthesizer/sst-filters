#ifndef SST_FILTERS_HALFRATEFILTER_H
#define SST_FILTERS_HALFRATEFILTER_H

#include <cstdint>
#include "sst/utilities/globals.h"

namespace sst::filters::HalfRate
{

static constexpr uint32_t halfrate_max_M = 6;
static constexpr uint32_t hr_BLOCK_SIZE = 256;

class alignas(16) HalfRateFilter
{
  private:
    // Remember leave these first so they stay aligned
    __m128 va[halfrate_max_M];
    __m128 vx0[halfrate_max_M];
    __m128 vx1[halfrate_max_M];
    __m128 vx2[halfrate_max_M];
    __m128 vy0[halfrate_max_M];
    __m128 vy1[halfrate_max_M];
    __m128 vy2[halfrate_max_M];
    __m128 oldout;

    const __m128 half = _mm_set_ps1(0.5f);

  public:
    /**
     * Implement a half rate up/down filter
     *
     * @param M The size of the FIR. Range from 1 to halfrate_max_M
     * @param steep Steepness. Set false for softer slopes, more attenuation and less stopband
     * ripple
     */
    HalfRateFilter(int M, bool steep)
    {
        assert(!(M > halfrate_max_M));
        this->M = M;
        this->steep = steep;
        load_coefficients();
        reset();
    }

    void process_block(float *floatL, float *floatR, int nsamples)
    {
        __m128 *__restrict L = (__m128 *)floatL;
        __m128 *__restrict R = (__m128 *)floatR;
        __m128 o[hr_BLOCK_SIZE];
        auto N = nsamples;
        // fill the buffer with interleaved stereo samples
        for (int k = 0; k < N; k += 4)
        {
            //[o3,o2,o1,o0] = [L0,L0,R0,R0]
            o[k] = _mm_shuffle_ps(L[k >> 2], R[k >> 2], _MM_SHUFFLE(0, 0, 0, 0));
            o[k + 1] = _mm_shuffle_ps(L[k >> 2], R[k >> 2], _MM_SHUFFLE(1, 1, 1, 1));
            o[k + 2] = _mm_shuffle_ps(L[k >> 2], R[k >> 2], _MM_SHUFFLE(2, 2, 2, 2));
            o[k + 3] = _mm_shuffle_ps(L[k >> 2], R[k >> 2], _MM_SHUFFLE(3, 3, 3, 3));
        }

        // process filters
        for (int j = 0; j < M; j++)
        {
            __m128 tx0 = vx0[j];
            __m128 tx1 = vx1[j];
            __m128 tx2 = vx2[j];
            __m128 ty0 = vy0[j];
            __m128 ty1 = vy1[j];
            __m128 ty2 = vy2[j];
            __m128 ta = va[j];

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
                ty0 = _mm_add_ps(tx2, _mm_mul_ps(_mm_sub_ps(tx0, ty2), ta));
                o[k] = ty0;

                // shuffle inputs
                tx2 = tx1;
                tx1 = tx0;
                tx0 = o[k + 1];
                // shuffle outputs
                ty2 = ty1;
                ty1 = ty0;
                // allpass filter 1
                ty0 = _mm_add_ps(tx2, _mm_mul_ps(_mm_sub_ps(tx0, ty2), ta));
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
        __m128 faR = _mm_setzero_ps();
        __m128 fbR = _mm_setzero_ps();

        for (int k = 0; k < N; k++)
        {
            //	const double output=(filter_a.process(input)+oldout)*0.5;
            //	oldout=filter_b.process(input);

            __m128 vL = _mm_add_ss(o[k], oldout);
            vL = _mm_mul_ss(vL, half);
            _mm_store_ss(&fL[k], vL);

            faR = _mm_movehl_ps(faR, o[k]);
            fbR = _mm_movehl_ps(fbR, oldout);

            __m128 vR = _mm_add_ss(faR, fbR);
            vR = _mm_mul_ss(vR, half);
            _mm_store_ss(&fR[k], vR);

            oldout = _mm_shuffle_ps(o[k], o[k], _MM_SHUFFLE(3, 3, 1, 1));
        }
    }
    void process_block_D2(float *floatL, float *floatR, int nsamples, float *outL = 0,
                          float *outR = 0) // process in-place. the new block will be half the size
    {
        __m128 *L = (__m128 *)floatL;
        __m128 *R = (__m128 *)floatR;
        __m128 o[hr_BLOCK_SIZE];

        /*
         * fill the buffer with interleaved stereo samples by rotating the
         * input simd-in-time a bit
         *
         * _mm_shuffle_ps(a,b,_MM_SHUFFLE(i,j,k,l)) returns a[i], a[j], b[k], b[l]
         *
         * So this loop makes o look like the rotation of L and R. That is
         *
         * o[n] = {L[n],L[n],R[n],R[n]}
         *
         * for n in 0..nsamples leading o to being LLRR pairs of the input
         */
        for (int k = 0; k < nsamples; k += 4)
        {
            o[k] = _mm_shuffle_ps(L[k >> 2], R[k >> 2], _MM_SHUFFLE(0, 0, 0, 0));
            o[k + 1] = _mm_shuffle_ps(L[k >> 2], R[k >> 2], _MM_SHUFFLE(1, 1, 1, 1));
            o[k + 2] = _mm_shuffle_ps(L[k >> 2], R[k >> 2], _MM_SHUFFLE(2, 2, 2, 2));
            o[k + 3] = _mm_shuffle_ps(L[k >> 2], R[k >> 2], _MM_SHUFFLE(3, 3, 3, 3));
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
        for (int j = 0; j < M; j++)
        {
            __m128 tx0 = vx0[j];
            __m128 tx1 = vx1[j];
            __m128 tx2 = vx2[j];
            __m128 ty0 = vy0[j];
            __m128 ty1 = vy1[j];
            __m128 ty2 = vy2[j];
            __m128 ta = va[j];

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
                ty0 = _mm_add_ps(tx2, _mm_mul_ps(_mm_sub_ps(tx0, ty2), ta));
                o[k] = ty0;

                // shuffle inputs
                tx2 = tx1;
                tx1 = tx0;
                tx0 = o[k + 1];
                // shuffle outputs
                ty2 = ty1;
                ty1 = ty0;
                // allpass filter 1
                ty0 = _mm_add_ps(tx2, _mm_mul_ps(_mm_sub_ps(tx0, ty2), ta));
                o[k + 1] = ty0;
            }
            vx0[j] = tx0;
            vx1[j] = tx1;
            vx2[j] = tx2;
            vy0[j] = ty0;
            vy1[j] = ty1;
            vy2[j] = ty2;
        }

        __m128 aR = _mm_setzero_ps();
        __m128 bR = _mm_setzero_ps();
        __m128 cR = _mm_setzero_ps();
        __m128 dR = _mm_setzero_ps();

        if (outL)
            L = (__m128 *)outL;
        if (outR)
            R = (__m128 *)outR;

        /*
         * OK so now we have all the filtered signals we want to reconstruct the output.
         * This is basically the sample selection stage. To read this code you need
         * to remember that _mm_movehl_ps(a,b) results in b[3], b[4], a[3], a[4] as the
         * simd output.
         *
         * The code had this comment
         *
         *  const double output=(filter_a.process(input)+oldout)*0.5;
         *  oldout=filter_b.process(input);
         *
         * atop this code
         *
         * __m128 tL0 = _mm_shuffle_ps(o[k], o[k], _MM_SHUFFLE(1, 1, 1, 1));
         * __m128 tR0 = _mm_shuffle_ps(o[k], o[k], _MM_SHUFFLE(3, 3, 3, 3));
         * __m128 aL = _mm_add_ss(tL0, o[k + 1]);
         * aR = _mm_movehl_ps(aR, o[k + 1]);
         * aR = _mm_add_ss(aR, tR0);
         *
         * So can we make that tie out? Remembering o now has the for B_L A_L, B_R, A_R
         *
         * So tL0 = _mm_shuffle_ps(o[k], o[k], 11111)
         * or tL0 = o[k][1] in every slot or tl0 is A_L across the board at sample k.
         * Similarly tR0 = A_R across the board at sample K.
         *
         * Now recall _mm_add_ss(a,b) gives you (a[0]+b[0], a[1], a[2], a[3]) so now we do
         *
         * __m128 aL = _mm_add_ss(tL0, o[k + 1]);
         * aL = (A_L[k] + B_L[k+1], A_L[k],  A_L[k], A_L[k]);
         *
         * Now
         *
         * aR = _mm_movehl_ps(aR, o[k + 1]);
         * aR = B_R[k+1], A_R[k+1], aR[3], aR[4]
         *
         * aR = _mm_add_ss(aR, tR0) or
         * aR = (A_R[k] + B_R[k+1], A_R[k+1], aR[3], aR[4])
         *
         * (At this point I'm suspecting that the rest of the SIMD registeres in A wont matter)
         *
         * Now similarly bL and bR, cL and cR, dL and dR are constructed with the same code just
         * at index k+2/3, k+4/5, k+6/7
         *
         * So once those stages are assembled we do this
         *
         * aL = _mm_movelh_ps(aL, bL);
         * cL = _mm_movelh_ps(cL, dL);
         * L[k >> 3] = _mm_shuffle_ps(aL, cL, _MM_SHUFFLE(2, 0, 2, 0));
         *
         * And similarly for R. So what's that doing. So first of all _mm_novelh_ps [note lh
         * not hl] has signatlre
         *
         * _mm_movelh_ps(a,b) = a[0],a[1],b[0],b[1]
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

            __m128 tL0 = _mm_shuffle_ps(o[k], o[k], _MM_SHUFFLE(1, 1, 1, 1));
            __m128 tR0 = _mm_shuffle_ps(o[k], o[k], _MM_SHUFFLE(3, 3, 3, 3));
            __m128 aL = _mm_add_ss(tL0, o[k + 1]);
            aR = _mm_movehl_ps(aR, o[k + 1]);
            aR = _mm_add_ss(aR, tR0);

            tL0 = _mm_shuffle_ps(o[k + 2], o[k + 2], _MM_SHUFFLE(1, 1, 1, 1));
            tR0 = _mm_shuffle_ps(o[k + 2], o[k + 2], _MM_SHUFFLE(3, 3, 3, 3));
            __m128 bL = _mm_add_ss(tL0, o[k + 3]);
            bR = _mm_movehl_ps(aR, o[k + 3]);
            bR = _mm_add_ss(bR, tR0);

            tL0 = _mm_shuffle_ps(o[k + 4], o[k + 4], _MM_SHUFFLE(1, 1, 1, 1));
            tR0 = _mm_shuffle_ps(o[k + 4], o[k + 4], _MM_SHUFFLE(3, 3, 3, 3));
            __m128 cL = _mm_add_ss(tL0, o[k + 5]);
            cR = _mm_movehl_ps(cR, o[k + 5]);
            cR = _mm_add_ss(cR, tR0);

            tL0 = _mm_shuffle_ps(o[k + 6], o[k + 6], _MM_SHUFFLE(1, 1, 1, 1));
            tR0 = _mm_shuffle_ps(o[k + 6], o[k + 6], _MM_SHUFFLE(3, 3, 3, 3));
            __m128 dL = _mm_add_ss(tL0, o[k + 7]);
            dR = _mm_movehl_ps(dR, o[k + 7]);
            dR = _mm_add_ss(dR, tR0);

            aL = _mm_movelh_ps(aL, bL);
            cL = _mm_movelh_ps(cL, dL);
            aR = _mm_movelh_ps(aR, bR);
            cR = _mm_movelh_ps(cR, dR);

            L[k >> 3] = _mm_shuffle_ps(aL, cL, _MM_SHUFFLE(2, 0, 2, 0));
            R[k >> 3] = _mm_shuffle_ps(aR, cR, _MM_SHUFFLE(2, 0, 2, 0));

            // optional: *=0.5;
            const __m128 half = _mm_set_ps1(0.5f);
            L[k >> 3] = _mm_mul_ps(L[k >> 3], half);
            R[k >> 3] = _mm_mul_ps(R[k >> 3], half);
        }
    }

    void process_block_U2(float *floatL_in, float *floatR_in, float *floatL, float *floatR,
                          int nsamples)
    {
        __m128 *L = (__m128 *)floatL;
        __m128 *R = (__m128 *)floatR;
        __m128 *L_in = (__m128 *)floatL_in;
        __m128 *R_in = (__m128 *)floatR_in;

        __m128 o[hr_BLOCK_SIZE];
        // fill the buffer with interleaved stereo samples
        for (int k = 0; k < nsamples; k += 8)
        {
            //[o3,o2,o1,o0] = [L0,L0,R0,R0]
            o[k] = _mm_shuffle_ps(L_in[k >> 3], R_in[k >> 3], _MM_SHUFFLE(0, 0, 0, 0));
            o[k + 1] = _mm_setzero_ps();
            o[k + 2] = _mm_shuffle_ps(L_in[k >> 3], R_in[k >> 3], _MM_SHUFFLE(1, 1, 1, 1));
            o[k + 3] = _mm_setzero_ps();
            o[k + 4] = _mm_shuffle_ps(L_in[k >> 3], R_in[k >> 3], _MM_SHUFFLE(2, 2, 2, 2));
            o[k + 5] = _mm_setzero_ps();
            o[k + 6] = _mm_shuffle_ps(L_in[k >> 3], R_in[k >> 3], _MM_SHUFFLE(3, 3, 3, 3));
            o[k + 7] = _mm_setzero_ps();
        }

        // process filters
        for (int j = 0; j < M; j++)
        {
            __m128 tx0 = vx0[j];
            __m128 tx1 = vx1[j];
            __m128 tx2 = vx2[j];
            __m128 ty0 = vy0[j];
            __m128 ty1 = vy1[j];
            __m128 ty2 = vy2[j];
            __m128 ta = va[j];

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
                ty0 = _mm_add_ps(tx2, _mm_mul_ps(_mm_sub_ps(tx0, ty2), ta));
                o[k] = ty0;

                // shuffle inputs
                tx2 = tx1;
                tx1 = tx0;
                tx0 = o[k + 1];
                // shuffle outputs
                ty2 = ty1;
                ty1 = ty0;
                // allpass filter 1
                ty0 = _mm_add_ps(tx2, _mm_mul_ps(_mm_sub_ps(tx0, ty2), ta));
                o[k + 1] = ty0;
            }
            vx0[j] = tx0;
            vx1[j] = tx1;
            vx2[j] = tx2;
            vy0[j] = ty0;
            vy1[j] = ty1;
            vy2[j] = ty2;
        }

        /*__m128 aR = _mm_setzero_ps();
        __m128 bR = _mm_setzero_ps();
        __m128 cR = _mm_setzero_ps();
        __m128 dR = _mm_setzero_ps();*/

        float *fL = (float *)L;
        float *fR = (float *)R;
        __m128 faR = _mm_setzero_ps();
        __m128 fbR = _mm_setzero_ps();

        for (int k = 0; k < nsamples; k++)
        {
            //	const double output=(filter_a.process(input)+oldout)*0.5;
            //	oldout=filter_b.process(input);

            __m128 vL = _mm_add_ss(o[k], oldout);
            vL = _mm_mul_ss(vL, half);
            _mm_store_ss(&fL[k], vL);

            faR = _mm_movehl_ps(faR, o[k]);
            fbR = _mm_movehl_ps(fbR, oldout);

            __m128 vR = _mm_add_ss(faR, fbR);
            vR = _mm_mul_ss(vR, half);
            _mm_store_ss(&fR[k], vR);

            oldout = _mm_shuffle_ps(o[k], o[k], _MM_SHUFFLE(3, 3, 1, 1));
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
        for (int i = 0; i < M; i++)
        {
            va[i] = _mm_setzero_ps();
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
        for (int i = 0; i < M; i++)
        {
            // va[i] = _mm_set_ps(cA[i],cB[i],cA[i],cB[i]);
            va[i] = _mm_set_ps(cB[i], cA[i], cB[i], cA[i]);
        }
    }
    void reset()
    {
        for (int i = 0; i < M; i++)
        {
            vx0[i] = _mm_setzero_ps();
            vx1[i] = _mm_setzero_ps();
            vx2[i] = _mm_setzero_ps();
            vy0[i] = _mm_setzero_ps();
            vy1[i] = _mm_setzero_ps();
            vy2[i] = _mm_setzero_ps();
        }
        oldout = _mm_setzero_ps();
    }

  private:
    int M;
    bool steep;
    float oldoutL, oldoutR;
    // unsigned int BLOCK_SIZE;
};

} // namespace sst::filters::HalfRate

#endif // SST_FILTERS_K35FILTER_H
