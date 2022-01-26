#include "TestUtils.h"

TEST_CASE("Diode Ladder")
{
    using namespace TestUtils;

    SECTION("Lowpass")
    {
        runTest({FilterType::fut_diode,
                 static_cast<FilterSubType>(0),
                 {-14.8965f, -5.12957f, -11.9454f, -18.757f, -38.9031f}});

        runTest({FilterType::fut_diode,
                 static_cast<FilterSubType>(1),
                 {-10.8158f, -3.45744f, -13.4409f, -24.9905f, -56.8347f}});

        runTest({FilterType::fut_diode,
                 static_cast<FilterSubType>(2),
                 {-8.81719f, -3.1469f, -16.3146f, -31.4996f, -57.6253f}});

        runTest({FilterType::fut_diode,
                 static_cast<FilterSubType>(3),
                 {-8.99764f, -4.00659f, -19.1205f, -35.4999f, -58.5961f}});
    }
}
