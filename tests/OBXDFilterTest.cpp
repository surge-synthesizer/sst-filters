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
                 {-33.7138f, -33.5791f, -33.3525f, -33.4417f, -33.4612f}});

        runTest({FilterType::fut_obxd_4pole,
                 static_cast<FilterSubType>(1),
                 {-33.8773f, -34.386f, -36.3865f, -41.315f, -61.9396f}});

        runTest({FilterType::fut_obxd_4pole,
                 static_cast<FilterSubType>(2),
                 {-34.0529f, -35.1998f, -39.3673f, -49.108f, -84.5386f}});

        runTest({FilterType::fut_obxd_4pole,
                 static_cast<FilterSubType>(3),
                 {-28.1586f, -29.7779f, -35.3439f, -47.2227f, -80.2969f}});
    }
}
