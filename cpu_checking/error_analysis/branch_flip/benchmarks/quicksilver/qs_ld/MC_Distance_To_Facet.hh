#ifndef MCT_DISTANCE_INCLUDE
#define MCT_DISTANCE_INCLUDE

#include "QS_Precision.hh"

#include "DeclareMacro.hh"

HOST_DEVICE_CLASS
class MC_Distance_To_Facet
{
public:
    qs_real distance;
    int facet;
    int subfacet;
    HOST_DEVICE_CUDA
    MC_Distance_To_Facet(): distance(qs_real(0.0)), facet(0), subfacet(0) {}
private:
    MC_Distance_To_Facet( const MC_Distance_To_Facet& );                    // disable copy constructor
    MC_Distance_To_Facet& operator=( const MC_Distance_To_Facet& tmp );     // disable assignment operator

};
HOST_DEVICE_END

#endif
