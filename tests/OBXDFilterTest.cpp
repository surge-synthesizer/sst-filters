#include "TestUtils.h"

TEST_CASE("OBXD Filter")
{
    using namespace TestUtils;

    SECTION("2-pole LPF")
    {
        runTest({FilterType::fut_obxd_2pole_lp,
                 static_cast<FilterSubType>(0),
                 {-5.42604f, -4.8885f, -5.72345f, -18.9727f, -54.1739f}});

        runTest({FilterType::fut_obxd_2pole_lp,
                 static_cast<FilterSubType>(1),
                 {-5.40362f, -4.74114f, -5.10849f, -18.8247f, -53.902f}});
    }

    SECTION("2-pole BPF")
    {
        runTest({FilterType::fut_obxd_2pole_bp,
                 static_cast<FilterSubType>(0),
                 {-26.4609f, -17.8167f, -11.7726f, -18.0352f, -40.1138f}});

        runTest({FilterType::fut_obxd_2pole_bp,
                 static_cast<FilterSubType>(1),
                 {-26.4323f, -17.6603f, -11.1551f, -17.8948f, -40.1103f}});
    }

    SECTION("2-pole HPF")
    {
        runTest({FilterType::fut_obxd_2pole_hp,
                 static_cast<FilterSubType>(0),
                 {-34.0961f, -18.4561f, -5.7608f, -4.90633f, -5.62153f}});

        runTest({FilterType::fut_obxd_2pole_hp,
                 static_cast<FilterSubType>(1),
                 {-34.0065f, -18.2937f, -5.14328f, -4.76908f, -5.62067f}});
    }

    SECTION("2-pole Notch")
    {
        runTest({FilterType::fut_obxd_2pole_n,
                 static_cast<FilterSubType>(0),
                 {-11.7372f, -12.9044f, -32.3445f, -12.7962f, -11.6544f}});

        runTest({FilterType::fut_obxd_2pole_n,
                 static_cast<FilterSubType>(1),
                 {-11.715f, -12.7591f, -32.0229f, -12.6596f, -11.6536f}});
    }

    SECTION("4-pole")
    {
        runTest({FilterType::fut_obxd_4pole,
                 static_cast<FilterSubType>(0),
                 {-10.3241f, -6.48075f, -0.536573f, -10.2352f, -30.8738f}});

        runTest({FilterType::fut_obxd_4pole,
                 static_cast<FilterSubType>(1),
                 {-10.4795f, -7.32661f, -3.53102f, -18.0087f, -53.1575f}});

        runTest({FilterType::fut_obxd_4pole,
                 static_cast<FilterSubType>(2),
                 {-10.6428f, -8.15069f, -6.50921f, -25.4671f, -56.6591f}});

        runTest({FilterType::fut_obxd_4pole,
                 static_cast<FilterSubType>(3),
                 {-4.7424f, -2.72974f, -2.51195f, -23.2202f, -51.9861f}});
    }
}
