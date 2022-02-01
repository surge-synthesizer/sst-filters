
#include "TestUtils.h"

TEST_CASE("Resonance Warp")
{
    using namespace TestUtils;

    SECTION("Lowpass")
    {
        std::array<RMSSet, 8> RMSs = {
            RMSSet{-11.1028f, -11.1612f, -11.3899f, -15.4922f, -48.2799f},
            RMSSet{-14.3007f, -14.2695f, -14.1346f, -26.8806f, -43.5797f},
            RMSSet{-16.1254f, -15.9804f, -15.1987f, -30.2618f, -37.6572f},
            RMSSet{-17.3687f, -17.1465f, -15.6886f, -27.8412f, -31.995f},
            RMSSet{-9.02908f, -9.01847f, -9.13351f, -15.4635f, -48.2445f},
            RMSSet{-11.64f, -11.5091f, -11.1657f, -25.6582f, -43.4708f},
            RMSSet{-13.2028f, -12.9716f, -11.9694f, -26.2522f, -37.3559f},
            RMSSet{-14.3004f, -14.0259f, -12.3487f, -23.5855f, -31.1887f},
        };

        for (int subtype = 0; subtype < 8; ++subtype)
            runTest({FilterType::fut_resonancewarp_lp, static_cast<FilterSubType>(subtype),
                     RMSs[subtype]});
    }

    SECTION("Highpass")
    {
        std::array<RMSSet, 8> RMSs = {
            RMSSet{-8.40693f, -8.06121f, -6.39376f, -2.57289f, -3.00098f},
            RMSSet{-16.9057f, -15.2773f, -9.89924f, -2.24694f, -2.98957f},
            RMSSet{-25.0511f, -19.0962f, -12.871f, -2.01657f, -2.97728f},
            RMSSet{-23.2052f, -16.0065f, -15.6979f, -1.86085f, -2.96191f},
            RMSSet{-10.4495f, -9.68903f, -6.73824f, -1.75938f, -3.00097f},
            RMSSet{-22.6055f, -18.0914f, -10.0022f, -0.733326f, -2.98953f},
            RMSSet{-23.6029f, -16.4363f, -12.7319f, 0.0172447f, -2.97705f},
            RMSSet{-20.1787f, -12.6086f, -13.3681f, 0.534672f, -2.96034f},
        };

        for (int subtype = 0; subtype < 8; ++subtype)
            runTest({FilterType::fut_resonancewarp_hp, static_cast<FilterSubType>(subtype),
                     RMSs[subtype]});
    }

    SECTION("Notch")
    {
        std::array<RMSSet, 8> RMSs = {
            RMSSet{-3.23448f, -5.95681f, -6.18723f, -3.48406f, -3.01332f},
            RMSSet{-3.44433f, -7.42709f, -10.3192f, -3.9265f, -3.01452f},
            RMSSet{-3.63701f, -8.50753f, -16.0515f, -4.3496f, -3.01574f},
            RMSSet{-3.8256f, -9.3087f, -23.5982f, -4.75651f, -3.01692f},
            RMSSet{-3.01936f, -4.20088f, -7.24986f, -3.36982f, -3.01331f},
            RMSSet{-3.11711f, -5.09661f, -13.282f, -3.70787f, -3.01451f},
            RMSSet{-3.2168f, -5.814f, -21.8745f, -4.03639f, -3.01572f},
            RMSSet{-3.31872f, -6.42808f, -26.4825f, -4.35773f, -3.01689f},
        };

        for (int subtype = 0; subtype < 8; ++subtype)
            runTest({FilterType::fut_resonancewarp_n, static_cast<FilterSubType>(subtype),
                     RMSs[subtype]});
    }

    SECTION("Bandpass")
    {
        std::array<RMSSet, 8> RMSs = {
            RMSSet{-8.2723f, -8.27821f, -8.64186f, -10.477f, -31.4317f},
            RMSSet{-13.2004f, -11.4246f, -11.5154f, -16.3005f, -44.6762f},
            RMSSet{-21.9316f, -14.8681f, -13.0118f, -21.7252f, -38.9506f},
            RMSSet{-22.0871f, -19.2751f, -13.77f, -26.2776f, -33.1328f},
            RMSSet{-7.23987f, -6.75755f, -6.99296f, -9.50604f, -31.4275f},
            RMSSet{-17.9678f, -9.76906f, -9.02468f, -15.1365f, -44.3042f},
            RMSSet{-19.0999f, -13.6225f, -10.0663f, -20.3868f, -38.4072f},
            RMSSet{-16.3908f, -18.5562f, -10.5933f, -24.1308f, -32.2261f},
        };

        for (int subtype = 0; subtype < 8; ++subtype)
            runTest({FilterType::fut_resonancewarp_bp, static_cast<FilterSubType>(subtype),
                     RMSs[subtype]});
    }

    SECTION("Allpass")
    {
        std::array<RMSSet, 8> RMSs = {
            RMSSet{-8.60903f, -8.74447f, -7.40205f, -4.22015f, -3.01233f},
            RMSSet{-12.897f, -15.3308f, -13.3802f, -5.26818f, -3.01262f},
            RMSSet{-13.203f, -17.0031f, -23.5072f, -6.18345f, -3.01287f},
            RMSSet{-13.4364f, -18.1184f, -25.6446f, -6.99239f, -3.01307f},
            RMSSet{-8.37051f, -10.3154f, -8.56746f, -3.8034f, -3.01229f},
            RMSSet{-9.30113f, -11.6858f, -18.0381f, -4.48343f, -3.01247f},
            RMSSet{-9.61528f, -12.8529f, -24.1465f, -5.08683f, -3.01266f},
            RMSSet{-9.87168f, -13.471f, -25.258f, -5.63353f, -3.0128f},
        };

        for (int subtype = 0; subtype < 8; ++subtype)
            runTest({FilterType::fut_resonancewarp_ap, static_cast<FilterSubType>(subtype),
                     RMSs[subtype]});
    }
}
