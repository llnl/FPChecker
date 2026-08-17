#ifndef DIRECTION_COSINE_INCLUDE
#define DIRECTION_COSINE_INCLUDE

#include "QS_Precision.hh"

#include <cmath>
#include "portability.hh"
#include "DeclareMacro.hh"

HOST_DEVICE_CLASS
class DirectionCosine
{
public:
   qs_real alpha;
   qs_real beta;
   qs_real gamma;

   HOST_DEVICE_CUDA
   DirectionCosine();

   HOST_DEVICE_CUDA
   DirectionCosine(qs_real alpha, qs_real beta, qs_real gamma);

   HOST_DEVICE_CUDA
   DirectionCosine &operator=(const DirectionCosine &dc) 
   {
       alpha = dc.alpha;
       beta  = dc.beta;
       gamma = dc.gamma;
       return *this;
    }

   void Sample_Isotropic(uint64_t *seed);

   // rotate a direction cosine given the sine/cosine of theta and phi
   HOST_DEVICE_CUDA
   inline void Rotate3DVector( qs_real sine_Theta,
                               qs_real cosine_Theta,
                               qs_real sine_Phi,
                               qs_real cosine_Phi );

};
HOST_DEVICE_END

HOST_DEVICE
inline DirectionCosine::DirectionCosine()
   : alpha(qs_real(0.0)), beta(qs_real(0.0)), gamma(qs_real(0.0))
{
}
HOST_DEVICE_END

HOST_DEVICE
inline DirectionCosine::DirectionCosine(qs_real a_alpha, qs_real a_beta, qs_real a_gamma)
   : alpha(a_alpha),
     beta(a_beta),
     gamma(a_gamma)
{
}
HOST_DEVICE_END

//----------------------------------------------------------------------------------------------------------------------
//
//  This function rotates a three-dimensional vector that is defined by the angles
//  Theta and Phi in a local coordinate frame about a polar angle and azimuthal angle described by
//  the direction cosine.  Hence, caller passes in sin_Theta and cos_Theta referenced from
//  the local z-axis and sin_Phi and cos_Phi referenced from the local x-axis to describe the
//  vector V to be rotated.  The direction cosine describes global theta and phi angles that the
//  vector V is to be rotated about.
//  Example:  Caller wishes to rotate a vector V described by direction cosines of (-1,0,0),
//            which corresponds to local angles Theta and Phi of
//            Theta =  Pi/2 so sin_Theta =  1, cos_Theta =  0,
//            Phi   = -Pi/2 so sin_Phi   = -1, cos_Phi   = -1. (wrong)
//            Phi   =  Pi   so sin_Phi   =  0, cos_Phi   = -1.
//
//            We wish to rotate this vector V by Pi/2 in global theta and Pi/2 in phi.
//            The resulting direction cosine is (alpha,beta,gamma) = (0,1,0).  The return result
//            is (1,0,0).  The rotation is about the forward y-axis followed by the (for) z-axis.
//
//            Note: this rotation operator fails when trying to rotate a vector V in the x-y plane
//            to another location in the x-y plane.  It essentially randomizes the rotation in x-y
//            instead of returning the exact rotation requested.
//
// ---------------------------- theory on the function
//
//        lowercase theta and phi are the spherical coordinates for direction_cosine.
//        in general spherical coordinates, phi is in the x,y plane, and theta is the angle with +z.
//        x = r qsm::cos_(phi) qsm::sin_(theta)
//        y = r qsm::sin_(phi) qsm::sin_(theta)
//        z = r qsm::cos_(theta)
//        or
//        r     = qsm::sqrt_(x*x + y*y + z*z)
//        phi   = qsm::atan_(y/x)
//        theta = qsm::acos_(z/r)
//
//        (x,y,z) = direction_cosine = (alpha, beta, gamma), with r = alpha^2 + beta^2 + gamma^2 = 1 so
//        alpha = qsm::cos_(phi) qsm::sin_(theta)
//        beta  = qsm::sin_(phi) qsm::sin_(theta)
//        gamma = qsm::cos_(theta)
//        or
//        phi   = qsm::atan_(beta/alpha)
//        theta = qsm::acos_(gamma)
//        so
//        qsm::sin_(phi) =  beta/qsm::sqrt_(alpha^2 + beta^2) =  beta/qsm::sqrt_(1 - gamma^2) =  beta/qsm::sin_(theta)
//        qsm::cos_(phi) = alpha/qsm::sqrt_(alpha^2 + beta^2) = alpha/qsm::sqrt_(1 - gamma^2) = alpha/qsm::sin_(theta)
//
//        Rotation matrix, lower case, times Upper Case Unit Vector.
//        The rotation matrix maps the x-axis (1,0,0) to the 1st column,
//                                     y-axis (0,1,0) to the 2nd column
//                                     z-axis (0,0,1) to the 3rd column.
//        (it maps the z-axis (0,0,1) to standard polar coordinates (qsm::cos_(phi)*qsm::sin_(theta), qsm::sin_(phi)*qsm::sin_(theta), qsm::cos_(theta))
//
//        [alpha] = [qsm::cos_(phi)*qsm::cos_(theta)   -qsm::sin_(phi)   qsm::cos_(phi)*qsm::sin_(theta)]  [qsm::sin_(Theta)*qsm::cos_(Phi)]
//        [beta ] = [qsm::sin_(phi)*qsm::cos_(theta)    qsm::cos_(phi)   qsm::sin_(phi)*qsm::sin_(theta)]  [qsm::sin_(Theta)*qsm::sin_(Phi)]
//        [gamma] = [        -qsm::sin_(theta)      0                 qsm::cos_(theta)]  [qsm::cos_(Theta)         ]
//
//        qs_real Alpha = sin_Theta*cos_Phi;
//        qs_real Beta  = sin_Theta*sin_Phi;
//        qs_real Gamma = cos_Theta;
//
//        direction_cosine.alpha =  cos_theta*cos_phi*Alpha - sin_phi*Beta + sin_theta*cos_phi*Gamma;
//        direction_cosine.beta =   cos_theta*sin_phi*Alpha + cos_phi*Beta + sin_theta*sin_phi*Gamma;
//        direction_cosine.gamma = -sin_theta        *Alpha +                cos_theta        *Gamma;
//----------------------------------------------------------------------------------------------------------------------
HOST_DEVICE
inline void DirectionCosine::Rotate3DVector(qs_real sin_Theta, qs_real cos_Theta, qs_real sin_Phi, qs_real cos_Phi)
{
    // Calculate additional variables in the rotation matrix.
    qs_real cos_theta = this->gamma;
    qs_real sin_theta = qsm::sqrt_((qs_real(1.0) - (cos_theta*cos_theta)));

    qs_real cos_phi;
    qs_real sin_phi;
    if (sin_theta < qs_real(1e-6)) // Order of qsm::sqrt_(PhysicalConstants::tiny_double)
    {
        cos_phi = qs_real(1.0); // assume phi  = 0.0;
        sin_phi = qs_real(0.0);
    }
    else
    {
        cos_phi = this->alpha/sin_theta;
        sin_phi = this->beta/sin_theta;
    }

    // Calculate the rotated direction cosine
    this->alpha =  cos_theta*cos_phi*(sin_Theta*cos_Phi) - sin_phi*(sin_Theta*sin_Phi) + sin_theta*cos_phi*cos_Theta;
    this->beta  =  cos_theta*sin_phi*(sin_Theta*cos_Phi) + cos_phi*(sin_Theta*sin_Phi) + sin_theta*sin_phi*cos_Theta;
    this->gamma = -sin_theta        *(sin_Theta*cos_Phi) +                               cos_theta        *cos_Theta;
}
HOST_DEVICE_END

#endif
