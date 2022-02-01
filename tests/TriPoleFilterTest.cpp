#include "TestUtils.h"

TEST_CASE("TriPole Filter")
{
    using namespace TestUtils;

    SECTION("First Stage")
    {
        runTest({FilterType::fut_tripole,
                 FilterSubType::st_tripole_LLL1,
                 {-4.25326f, -6.37334f, -13.6173f, -17.6295f, -20.9406f}});

        runTest({FilterType::fut_tripole,
                 FilterSubType::st_tripole_LHL1,
                 {-8.15925f, -8.96029f, -10.9976f, -17.4781f, -20.1552f}});

        runTest({FilterType::fut_tripole,
                 FilterSubType::st_tripole_HLH1,
                 {-17.3739f, -8.23246f, -3.939f, -2.9143f, -3.07959f}});

        runTest({FilterType::fut_tripole,
                 FilterSubType::st_tripole_HHH1,
                 {-15.6124f, -6.58177f, -4.77551f, -3.50092f, -3.09306f}});
    }

    SECTION("Second Stage")
    {
        runTest({FilterType::fut_tripole,
                 FilterSubType::st_tripole_LLL2,
                 {-8.39167f, -12.4356f, -19.2612f, -21.1454f, -22.5948f}});

        runTest({FilterType::fut_tripole,
                 FilterSubType::st_tripole_LHL2,
                 {-22.0593f, -14.0584f, -12.8537f, -18.3194f, -22.1564f}});

        runTest({FilterType::fut_tripole,
                 FilterSubType::st_tripole_HLH2,
                 {-17.5901f, -12.9352f, -15.7992f, -21.2519f, -38.8294f}});

        runTest({FilterType::fut_tripole,
                 FilterSubType::st_tripole_HHH2,
                 {-29.9571f, -11.3739f, -5.64681f, -3.70194f, -3.17114f}});
    }

    SECTION("Third Stage")
    {
        runTest({FilterType::fut_tripole,
                 FilterSubType::st_tripole_LLL3,
                 {-18.5412f, -19.5961f, -22.0208f, -19.3382f, -18.5305f}});

        runTest({FilterType::fut_tripole,
                 FilterSubType::st_tripole_LHL3,
                 {-17.9264f, -19.5055f, -22.491f, -27.3633f, -23.4616f}});

        runTest({FilterType::fut_tripole,
                 FilterSubType::st_tripole_HLH3,
                 {-28.0098f, -22.5628f, -19.7113f, -18.2213f, -18.279f}});

        runTest({FilterType::fut_tripole,
                 FilterSubType::st_tripole_HHH3,
                 {-30.506f, -22.8295f, -19.6699f, -18.9364f, -22.5596f}});
    }
}
