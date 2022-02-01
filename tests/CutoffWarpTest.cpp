#include "TestUtils.h"

TEST_CASE("Cutoff Warp")
{
    using namespace TestUtils;

    SECTION("Lowpass")
    {
        std::array<RMSSet, 12> RMSs = {
            RMSSet{-8.42754f, -8.46018f, -8.64071f, -6.48595f, -41.2379f},
            RMSSet{-12.2309f, -12.353f, -13.0801f, -8.70028f, -32.7409f},
            RMSSet{-16.5502f, -16.8967f, -15.9939f, -16.2986f, -29.2027f},
            RMSSet{-19.2632f, -18.5471f, -17.7021f, -26.5587f, -27.35f},
            RMSSet{-7.98588f, -7.94194f, -8.27965f, -5.39509f, -42.735f},
            RMSSet{-13.9832f, -14.0219f, -13.7094f, -15.7409f, -38.0127f},
            RMSSet{-18.9166f, -18.1599f, -16.9123f, -28.4242f, -34.7035f},
            RMSSet{-22.5712f, -19.1603f, -19.0436f, -29.386f, -32.5895f},
            RMSSet{-0.473666f, -1.03385f, -2.12155f, -10.9423f, -43.9252f},
            RMSSet{-2.17443f, -3.20736f, -4.08984f, -17.099f, -39.3575f},
            RMSSet{-4.23961f, -6.01578f, -4.5833f, -17.0252f, -35.8984f},
            RMSSet{-5.44275f, -7.92927f, -5.37538f, -14.6903f, -30.4821f},
        };

        for (int subtype = 0; subtype < 12; ++subtype)
            runTest({FilterType::fut_cutoffwarp_lp, static_cast<FilterSubType>(subtype),
                     RMSs[subtype]});
    }

    SECTION("Highpass")
    {
        std::array<RMSSet, 12> RMSs = {
            RMSSet{-27.9408f, -17.6228f, -12.0602f, -6.53909f, 5.56637f},
            RMSSet{-28.9466f, -19.951f, -10.0301f, -8.75524f, 12.5246f},
            RMSSet{-27.9723f, -18.9552f, -11.12f, -11.145f, 14.9592f},
            RMSSet{-27.3802f, -18.7229f, -11.0803f, -9.52532f, 17.2771f},
            RMSSet{-28.9733f, -17.5357f, -11.6588f, -5.7755f, 1.28922f},
            RMSSet{-32.5947f, -23.6271f, -17.4252f, -9.67573f, 4.86942f},
            RMSSet{-32.1665f, -23.4867f, -20.1576f, -11.3581f, -2.42279f},
            RMSSet{-32.4493f, -24.4624f, -22.221f, -13.106f, -12.4986f},
            RMSSet{-29.4439f, -14.0263f, -5.67273f, -2.35983f, 0.263967f},
            RMSSet{-33.0063f, -23.2899f, -8.15209f, -5.65788f, 2.9865f},
            RMSSet{-32.8913f, -24.5376f, -12.8864f, -6.88203f, -2.91462f},
            RMSSet{-30.8666f, -22.7357f, -12.8647f, -8.59865f, -9.88702f},
        };

        for (int subtype = 0; subtype < 12; ++subtype)
            runTest({FilterType::fut_cutoffwarp_hp, static_cast<FilterSubType>(subtype),
                     RMSs[subtype]});
    }

    SECTION("Notch")
    {
        std::array<RMSSet, 12> RMSs = {
            RMSSet{-15.7139f, -16.3907f, -29.8196f, -10.7688f, 1.80881f},
            RMSSet{-21.3121f, -22.4172f, -27.3244f, -12.4116f, 7.83475f},
            RMSSet{-24.5152f, -25.254f, -25.0563f, -11.8337f, 13.3971f},
            RMSSet{-26.6177f, -25.2095f, -23.1863f, -11.7511f, 18.3887f},
            RMSSet{-13.6704f, -14.3377f, -27.764f, -8.32607f, -0.857985f},
            RMSSet{-18.5938f, -19.7693f, -25.2047f, -9.43582f, 6.20573f},
            RMSSet{-21.5289f, -22.565f, -22.8505f, -8.66339f, 11.3409f},
            RMSSet{-23.5299f, -23.8011f, -20.9557f, -8.52401f, 15.6855f},
            RMSSet{-4.92377f, -6.23941f, -20.2534f, -3.40778f, -0.613433f},
            RMSSet{-5.18233f, -6.94286f, -23.1431f, -4.22522f, 6.4151f},
            RMSSet{-5.33815f, -7.56541f, -24.3141f, -4.09373f, 11.3093f},
            RMSSet{-5.4932f, -8.10476f, -25.0426f, -3.67f, 15.9409f},
        };

        for (int subtype = 0; subtype < 12; ++subtype)
            runTest(
                {FilterType::fut_cutoffwarp_n, static_cast<FilterSubType>(subtype), RMSs[subtype]});
    }

    SECTION("Bandpass")
    {
        std::array<RMSSet, 12> RMSs = {
            RMSSet{-22.5685f, -18.5005f, -15.7775f, -12.0794f, -31.4218f},
            RMSSet{-33.882f, -24.7089f, -19.5782f, -14.0066f, -44.3671f},
            RMSSet{-32.4093f, -21.9859f, -21.3295f, -14.5973f, -38.6115f},
            RMSSet{-28.6119f, -20.0841f, -22.3234f, -14.8714f, -33.3811f},
            RMSSet{-21.0908f, -16.53f, -13.8123f, -9.49571f, -31.4228f},
            RMSSet{-31.9273f, -22.4531f, -16.9259f, -10.8078f, -44.1187f},
            RMSSet{-29.6541f, -19.9918f, -18.107f, -11.2645f, -38.1888f},
            RMSSet{-26.1311f, -17.0563f, -18.769f, -11.5235f, -32.2333f},
            RMSSet{-17.6009f, -9.26712f, -6.47248f, -4.62371f, -31.4237f},
            RMSSet{-27.468f, -13.8478f, -6.91091f, -8.08413f, -43.9883f},
            RMSSet{-23.629f, -13.381f, -6.58062f, -8.37814f, -37.9282f},
            RMSSet{-18.8344f, -10.3469f, -7.01664f, -6.83151f, -31.2991f},
        };

        for (int subtype = 0; subtype < 12; ++subtype)
            runTest({FilterType::fut_cutoffwarp_bp, static_cast<FilterSubType>(subtype),
                     RMSs[subtype]});
    }

    SECTION("Allpass")
    {
        std::array<RMSSet, 12> RMSs = {
            RMSSet{-15.7187f, -16.2958f, -18.3702f, -10.7129f, 1.81162f},
            RMSSet{-21.4081f, -22.5721f, -22.2541f, -11.9247f, 7.8139f},
            RMSSet{-24.5918f, -25.1436f, -21.7172f, -11.8863f, 13.3243f},
            RMSSet{-26.5394f, -24.7197f, -20.4244f, -11.9322f, 18.3311f},
            RMSSet{-13.6754f, -14.2352f, -15.981f, -8.27873f, -0.855268f},
            RMSSet{-18.6854f, -19.9836f, -19.6175f, -9.00041f, 6.19664f},
            RMSSet{-21.6384f, -22.5007f, -19.1871f, -8.75561f, 11.3107f},
            RMSSet{-23.4818f, -22.4977f, -17.8829f, -9.12251f, 15.7192f},
            RMSSet{-4.9321f, -6.22004f, -8.64441f, -3.37268f, -0.60962f},
            RMSSet{-5.23879f, -7.05682f, -9.0091f, -4.54469f, 6.40074f},
            RMSSet{-5.51873f, -7.66597f, -9.50421f, -5.43729f, 11.4305f},
            RMSSet{-5.73764f, -8.29928f, -9.89278f, -3.72915f, 15.9523f},
        };

        for (int subtype = 0; subtype < 12; ++subtype)
            runTest({FilterType::fut_cutoffwarp_ap, static_cast<FilterSubType>(subtype),
                     RMSs[subtype]});
    }
}
