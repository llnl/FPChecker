#ifndef MC_VECTOR_INCLUDE
#define MC_VECTOR_INCLUDE

#include "QS_Precision.hh"

#include <cmath>
#include "DeclareMacro.hh"

HOST_DEVICE_CLASS
class MC_Vector
{
 public:
   qs_real x;
   qs_real y;
   qs_real z;

   HOST_DEVICE_CUDA
   MC_Vector() : x(0), y(0), z(0) {}
   HOST_DEVICE_CUDA
   MC_Vector(qs_real a, qs_real b, qs_real c) : x(a), y(b), z(c) {}

   HOST_DEVICE_CUDA
   MC_Vector& operator=( const MC_Vector&tmp )
   {
      if ( this == &tmp ) { return *this; }

      x = tmp.x;
      y = tmp.y;
      z = tmp.z;

      return *this;
   }

   HOST_DEVICE_CUDA
   bool operator==( const MC_Vector& tmp )
   {
      return tmp.x == x && tmp.y == y && tmp.z == z;
   }

   HOST_DEVICE_CUDA
   MC_Vector& operator+=( const MC_Vector &tmp )
   {
      x += tmp.x;
      y += tmp.y;
      z += tmp.z;
      return *this;
   }

   HOST_DEVICE_CUDA
   MC_Vector& operator-=( const MC_Vector &tmp )
   {
      x -= tmp.x;
      y -= tmp.y;
      z -= tmp.z;
      return *this;
   }

   HOST_DEVICE_CUDA
   MC_Vector& operator*=(const qs_real scalar)
   {
      x *= scalar;
      y *= scalar;
      z *= scalar;
      return *this;
   }

   HOST_DEVICE_CUDA
   MC_Vector& operator/=(const qs_real scalar)
   {
      x /= scalar;
      y /= scalar;
      z /= scalar;
      return *this;
   }

   HOST_DEVICE_CUDA
   const MC_Vector operator+( const MC_Vector &tmp ) const
   {
      return MC_Vector(x + tmp.x, y + tmp.y, z + tmp.z);
   }

   HOST_DEVICE_CUDA
   const MC_Vector operator-( const MC_Vector &tmp ) const
   {
      return MC_Vector(x - tmp.x, y - tmp.y, z - tmp.z);
   }

   HOST_DEVICE_CUDA
   const MC_Vector operator*(const qs_real scalar) const
   {
      return MC_Vector(scalar*x, scalar*y, scalar*z);
   }

   HOST_DEVICE_CUDA
   inline qs_real Length() const { return qsm::sqrt_(x*x + y*y + z*z); }

   // Distance from this vector to another point.
   HOST_DEVICE_CUDA
   inline qs_real Distance(const MC_Vector& vv) const
   { return qsm::sqrt_((x - vv.x)*(x - vv.x) + (y - vv.y)*(y - vv.y)+ (z - vv.z)*(z - vv.z)); }

   HOST_DEVICE_CUDA
   inline qs_real Dot(const MC_Vector &tmp) const
   {
      return this->x*tmp.x + this->y*tmp.y + this->z*tmp.z;
   }

   HOST_DEVICE_CUDA
   inline MC_Vector Cross(const MC_Vector &v) const
   {
      return MC_Vector(y * v.z - z * v.y,
                       z * v.x - x * v.z,
                       x * v.y - y * v.x);
   }

};
HOST_DEVICE_END


#endif
