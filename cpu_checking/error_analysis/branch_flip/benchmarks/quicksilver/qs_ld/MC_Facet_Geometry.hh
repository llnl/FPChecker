#ifndef MCT_FACET_GEOMETRY_3D_INCLUDE
#define MCT_FACET_GEOMETRY_3D_INCLUDE

#include "QS_Precision.hh"

#include "macros.hh"
#include <cstddef> // NULL

// A x + B y + C z + D = 0,  (A,B,C) is the plane normal and is normalized.
class MC_General_Plane
{
public:
    qs_real A;
    qs_real B;
    qs_real C;
    qs_real D;

   // Code to compute coefficients stolen from MCT_Facet_Adjacency_3D_G
   MC_General_Plane(){};
   MC_General_Plane(const MC_Vector& r0, const MC_Vector& r1, const MC_Vector& r2)
   {
      A = ((r1.y - r0.y)*(r2.z - r0.z)) - ((r1.z - r0.z)*(r2.y - r0.y));
      B = ((r1.z - r0.z)*(r2.x - r0.x)) - ((r1.x - r0.x)*(r2.z - r0.z));
      C = ((r1.x - r0.x)*(r2.y - r0.y)) - ((r1.y - r0.y)*(r2.x - r0.x));
      D = -qs_real(1.0)*(A*r0.x + B*r0.y + C*r0.z);

      qs_real magnitude = qsm::sqrt_(A * A + B * B + C * C);

      if ( magnitude == qs_real(0.0) )
      {
         A = qs_real(1.0);
         magnitude = qs_real(1.0);
      }
      // Normalize the planar-facet geometric cofficients.
      qs_real inv_denominator = qs_real(1.0) / magnitude;

      A *= inv_denominator;
      B *= inv_denominator;
      C *= inv_denominator;
      D *= inv_denominator;
   }

};


class MC_Facet_Geometry_Cell
{
 public:
   MC_General_Plane* _facet;
   int _size;
};

#endif
