#ifndef MCT_NEAREST_FACET_INCLUDE
#define MCT_NEAREST_FACET_INCLUDE

#include "QS_Precision.hh"

#include "DeclareMacro.hh"

class MC_Nearest_Facet
{
 public:

   int    facet;
   qs_real distance_to_facet;
   qs_real dot_product;

   HOST_DEVICE
   MC_Nearest_Facet()
   : facet(0),
     distance_to_facet(qs_real(1e80)),
     dot_product(qs_real(0.0))
   {}

   HOST_DEVICE_CUDA
   MC_Nearest_Facet& operator=( const MC_Nearest_Facet& nf )
   {
        this->facet             = nf.facet;
        this->distance_to_facet = nf.distance_to_facet;
        this->dot_product       = nf.dot_product;
        return *this;
   }
   HOST_DEVICE_END

};
#endif
