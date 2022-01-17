#define CATCH_CONFIG_RUNNER
#include "catch2/catch2.hpp"

#include <sst/filters.h>

/*
 * This entire file obviously will be replaced
 */
struct SamplePitchMaker
{
    float note_to_pitch(float note) { return pow(2.0, note / 12.f); }
};

TEST_CASE("Basic")
{
    SECTION("Coefficient Maker")
    {
        auto spm = std::make_unique<SamplePitchMaker>();
        REQUIRE(sst::filters::CoefficientMaker<SamplePitchMaker>::pitch(spm.get(), 0) == 1.0);
        REQUIRE(sst::filters::CoefficientMaker<SamplePitchMaker>::pitch(spm.get(), 60) == 32.0);
    }

    SECTION("Function") { REQUIRE(sst::filters::makeFilter(sst::filters::ft_off, 0) == nullptr); }
}
int main(int argc, char **argv)
{
    int result = Catch::Session().run(argc, argv);
    return result;
}
