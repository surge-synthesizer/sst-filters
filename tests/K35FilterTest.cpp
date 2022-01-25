#include "TestUtils.h"

TEST_CASE("K35 Filter")
{
    using namespace TestUtils;

    SECTION("Lowpass")
    {
        runTest({FilterType::fut_k35_lp,
                 static_cast<FilterSubType>(0),
                 {-2.81718f, -2.3147f, -3.25914f, -16.3992f, -51.6302f}});

        runTest({FilterType::fut_k35_lp,
                 static_cast<FilterSubType>(1),
                 {-4.71353f, -4.90443f, -7.11085f, -17.0842f, -51.6413f}});

        runTest({FilterType::fut_k35_lp,
                 static_cast<FilterSubType>(2),
                 {-1.77084f, -2.23616f, -4.49667f, -11.9291f, -36.3152f}});

        runTest({FilterType::fut_k35_lp,
                 static_cast<FilterSubType>(3),
                 {-0.952249f, -1.73672f, -4.06241f, -10.24f, -6.31845f}});

        runTest({FilterType::fut_k35_lp,
                 static_cast<FilterSubType>(4),
                 {-0.711815f, -1.60224f, -3.92835f, -9.56674f, -4.65181f}});
    }

    SECTION("Highpass")
    {
        runTest({FilterType::fut_k35_hp,
                 static_cast<FilterSubType>(0),
                 {-17.6108f, -8.32157f, -0.245684f, -1.55808f, -3.00011f}});

        runTest({FilterType::fut_k35_hp,
                 static_cast<FilterSubType>(1),
                 {-17.6865f, -8.99833f, -3.98583f, -4.07719f, -4.71325f}});

        runTest({FilterType::fut_k35_hp,
                 static_cast<FilterSubType>(2),
                 {-11.7006f, -3.50698f, -1.15109f, -1.4057f, -1.81198f}});

        runTest({FilterType::fut_k35_hp,
                 static_cast<FilterSubType>(3),
                 {-3.39694f, -1.48648f, -0.576566f, -0.761196f, -0.984327f}});

        runTest({FilterType::fut_k35_hp,
                 static_cast<FilterSubType>(4),
                 {-1.66492f, -0.724319f, -0.357077f, -0.50091f, -0.640297f}});
    }
}
