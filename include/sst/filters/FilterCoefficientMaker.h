#ifndef SST_FILTERS_FILTERCOEFFICIENTMAKER_H
#define SST_FILTERS_FILTERCOEFFICIENTMAKER_H

#include "FilterConfiguration.h"

namespace sst
{
namespace filters
{

constexpr int n_cm_coeffs = 8;

template <typename TuningProvider> class FilterCoefficientMaker
{
  public:
    /** Default constructor */
    FilterCoefficientMaker();

    /** Sets the sample rate and block size to use for processing the filters */
    void setSampleRateAndBlockSize(float newSampleRate,
                                   int newBlockSize); // @NOTE: for Surge, this should be called
                                                      // with samplerate_os, and BLOCK_SIZE_OS

    /** Resets the coefficients to zero, and the tuning provider */
    void Reset();

    /** Creates filter coefficients directly from an array */
    void FromDirect(const float (&N)[n_cm_coeffs]);

    /** Creates filter coefficients for a given set of filter parameters */
    void MakeCoeffs(float Freq, float Reso, FilterType Type, int SubType);

    /** Creates filter coefficients for a given set of filter parameters */
    void MakeCoeffs(float Freq, float Reso, FilterType Type, int SubType, TuningProvider *provider,
                    bool tuningAdjusted);

    void castCoefficients(__m128 (&C)[n_cm_coeffs], __m128 (&dC)[n_cm_coeffs]);

    /** Current filter coefficients */
    float C[n_cm_coeffs]{};

    /** Filter coefficients "delta" to update current coefficients */
    float dC[n_cm_coeffs]{};

    /** "Target" filter coefficients */
    float tC[n_cm_coeffs]{};

  private:
    void ToCoupledForm(double A0inv, double A1, double A2, double B0, double B1, double B2,
                       double G);
    void ToNormalizedLattice(double A0inv, double A1, double A2, double B0, double B1, double B2,
                             double G);
    void Coeff_LP12(float Freq, float Reso, int SubType);
    void Coeff_HP12(float Freq, float Reso, int SubType);
    void Coeff_BP12(float Freq, float Reso, int SubType);
    void Coeff_Notch(float Freq, float Reso, int SubType);
    void Coeff_APF(float Freq, float Reso, int SubType);
    void Coeff_LP24(float Freq, float Reso, int SubType);
    void Coeff_HP24(float Freq, float Reso, int SubType);
    void Coeff_BP24(float Freq, float Reso, int SubType);
    void Coeff_LP4L(float Freq, float Reso, int SubType);
    void Coeff_COMB(float Freq, float Reso, int SubType);
    void Coeff_SNH(float Freq, float Reso, int SubType);
    void Coeff_SVF(float Freq, float Reso, bool);

    bool FirstRun = true;

    TuningProvider *provider = nullptr;

    float sampleRate = 48000.0f;
    float sampleRateInv = 1.0f / sampleRate;

    int blockSize = 32;
    float blockSizeInv = 1.0f / (float)blockSize;
};

} // namespace filters
} // namespace sst

#include "FilterCoefficientMaker_Impl.h"

#endif // SST_FILTERS_FILTERCOEFFICIENTMAKER_H
