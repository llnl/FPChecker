#include "DirectionCosine.hh"
#include "QS_Precision.hh"
#include "MC_RNG_State.hh"
#include "PhysicalConstants.hh"

void DirectionCosine::Sample_Isotropic(uint64_t *seed)
{
    this->gamma  = qs_real(1.0) - qs_real(2.0)*rngSample(seed);
    qs_real sine_gamma  = qsm::sqrt_((qs_real(1.0) - (gamma*gamma)));
    qs_real phi         = PhysicalConstants::_pi*(qs_real(2.0)*rngSample(seed) - qs_real(1.0));

    this->alpha  = sine_gamma * qsm::cos_(phi);
    this->beta   = sine_gamma * qsm::sin_(phi);
}
