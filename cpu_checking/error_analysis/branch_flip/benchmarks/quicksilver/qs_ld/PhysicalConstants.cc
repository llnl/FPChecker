#include "PhysicalConstants.hh"
#include "QS_Precision.hh"

   // The values of all physical constants are taken from:
   // 2006 CODATA which is located on the web at
   // http://physics.nist.gov/cuu/Constants/codata.pdf

   // The units of physical quantities used by the code are:
   //    Mass         -  gram (g)
   //    Length       -  centimeter (cm)
   //    Time         -  second (s)
   //    Energy       -  million electron-volts (MeV) : of a particle
   //    Energy       -  erg (g cm^2/s^2): in some background calculation
   //    Temperature  -  thousand electron-volts (keV)

const qs_real PhysicalConstants::_neutronRestMassEnergy = qs_real(9.395656981095e+2); /* MeV */
const qs_real PhysicalConstants::_pi = qs_real(3.1415926535897932);
const qs_real PhysicalConstants::_speedOfLight  = qs_real(2.99792458e+10);                // cm / s

// Constants used in math for computer science, roundoff, and other reasons
const qs_real PhysicalConstants::_tinyDouble           = qs_sentinel::tiny_;
const qs_real PhysicalConstants::_smallDouble          = qs_sentinel::small_;
const qs_real PhysicalConstants::_hugeDouble           = qs_sentinel::huge_;
