#ifndef MC_RNG_STATE_INCLUDE
#define MC_RNG_STATE_INCLUDE

#include "QS_Precision.hh"

#include "portability.hh"
#include "DeclareMacro.hh"

//----------------------------------------------------------------------------------------------------------------------
//  A random number generator that implements a 64 bit linear congruential generator (lcg).
//
//  This implementation is based on the rng class from Nick Gentile.
//----------------------------------------------------------------------------------------------------------------------

// Generate a new random number seed
HOST_DEVICE
uint64_t rngSpawn_Random_Number_Seed(uint64_t *parent_seed);
HOST_DEVICE_END

//----------------------------------------------------------------------------------------------------------------------
//  Sample returns the pseudo-random number produced by a call to a random
//  number generator.
//----------------------------------------------------------------------------------------------------------------------
HOST_DEVICE
inline qs_real rngSample(uint64_t *seed)
{
   // Reset the state from the previous value.
   // NOTE: this LCG is pure 64-bit integer arithmetic, so the seed
   // stream is BIT-IDENTICAL across fp32/fp64/ld.  Particle histories
   // therefore start from identical draws in every tree.
   *seed = 2862933555777941757ULL*(*seed) + 3037000493ULL;

   // Map the int state in (0,2**64) to qs_real (0,1)
   // by multiplying by
   // 1/(2**64 - 1) = 1/18446744073709551615.
#if QS_RNG_NATIVE
   return qs_real(5.4210108624275222e-20)*qs_real(*seed);
#else
   // Map in double, round once to working precision: the cast is the
   // sole precision boundary of the generator.
   return qs_real(5.4210108624275222e-20*double(*seed));
#endif
}
HOST_DEVICE_END

#endif
