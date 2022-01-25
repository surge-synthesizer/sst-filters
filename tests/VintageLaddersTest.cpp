#include "TestUtils.h"

TEST_CASE("Vintage Ladders")
{
    using namespace TestUtils;

    SECTION("Vintage Ladder RK")
    {
        runTest({FilterType::fut_vintageladder,
                 static_cast<FilterSubType>(0),
                 {-17.9486f, -16.0861f, -13.4772f, -37.4639f, -64.0226f}});

        runTest({FilterType::fut_vintageladder,
                 static_cast<FilterSubType>(1),
                 {-10.7992f, -8.91631f, -8.36544f, -30.9535f, -57.8073f}});
    }

    SECTION("Vintage Ladder Huov")
    {
        runTest({FilterType::fut_vintageladder,
                 static_cast<FilterSubType>(2),
                 {-12.251f, -10.3796f, -9.22999f, -32.5839f, -59.1211f}});

        runTest({FilterType::fut_vintageladder,
                 static_cast<FilterSubType>(3),
                 {-6.17262f, -4.30116f, -3.1663f, -26.5073f, -53.0447f}});
    }
}
