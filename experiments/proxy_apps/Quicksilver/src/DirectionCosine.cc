#include "DirectionCosine.hh"
#include "MC_RNG_State.hh"
#include "PhysicalConstants.hh"

void DirectionCosine::Sample_Isotropic(uint64_t *seed)
{
    this->gamma  = 1.0 - 2.0*rngSample(seed);
    double sine_gamma  = sqrt((1.0 - (gamma*gamma)));
    double phi         = PhysicalConstants::_pi*(2.0*rngSample(seed) - 1.0);

#ifndef FPC_QUICKSILVER_DISABLE_INJECTION
    {
        double state_x = (this->gamma + sine_gamma + phi + 1.0) * 123456.789;
        double state_y = state_x + state_x * 1.0e-15;
        volatile double state_cancelled_result __attribute__((unused)) = state_y - state_x; // Injection state
    }
#endif

    this->alpha  = sine_gamma * cos(phi);
    this->beta   = sine_gamma * sin(phi);
}
