#ifndef PHYSICAL_CONSTANTS_HH
#define PHYSICAL_CONSTANTS_HH

#include "QS_Precision.hh"

#include "DeclareMacro.hh"
HOST_DEVICE_CLASS
namespace PhysicalConstants
{

const qs_real _neutronRestMassEnergy = qs_real(9.395656981095e+2); /* MeV */
const qs_real _pi = qs_real(3.1415926535897932);
const qs_real _speedOfLight  = qs_real(2.99792458e+10);                // cm / s

// Constants used in math for computer science, roundoff, and other reasons
 const qs_real _tinyDouble           = qs_sentinel::tiny_;
 const qs_real _smallDouble          = qs_sentinel::small_;
 const qs_real _hugeDouble           = qs_sentinel::huge_;   // 1e75 overflows float
//
}
HOST_DEVICE_END


#endif
