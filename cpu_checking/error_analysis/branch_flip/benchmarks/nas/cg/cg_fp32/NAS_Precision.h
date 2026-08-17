#ifndef NAS_PRECISION_H
#define NAS_PRECISION_H

/*=====================================================================
 *  NAS Parallel Benchmarks -- working-precision control.
 *
 *  This header is BYTE-IDENTICAL in every benchmark and every tree.
 *  Precision is selected at COMPILE TIME, so all three builds come from
 *  ONE byte-identical source tree:
 *
 *      -DNAS_FP32   ->  float
 *      -DNAS_LD     ->  long double
 *      (nothing)    ->  double
 *
 *  Matches the -DLULESH_FP32 / -DHYPRE_SINGLE / -DQS_FP32 idiom used for
 *  the other three benchmarks.
 *
 *  Replaces the earlier recipe, which produced three separately-edited
 *  copies differing in ~1290 lines per benchmark.  A single tree makes
 *  the matched-build invariant checkable by diff.
 *===================================================================*/

#include <math.h>
#include <float.h>

/*---------------------------------------------------------------------
 * THE PRECISION SWITCH
 *-------------------------------------------------------------------*/
#if   defined(NAS_FP32)
typedef float       nas_real;
#define NAS_PRECISION_NAME "fp32"
#define NAS_REAL_EPSILON   FLT_EPSILON

#elif defined(NAS_LD)
typedef long double nas_real;
#define NAS_PRECISION_NAME "ld"
#define NAS_REAL_EPSILON   LDBL_EPSILON

#else /* default */
typedef double      nas_real;
#define NAS_PRECISION_NAME "fp64"
#define NAS_REAL_EPSILON   DBL_EPSILON
#endif

/*---------------------------------------------------------------------
 * Explicit libm dispatch.
 *
 * C has no overloading: a bare sqrt() is always double sqrt(double).
 * Under NAS_FP32 the argument is promoted, computed in double, and
 * rounded back -- so the "fp32" build is not really fp32 at those sites.
 * Under NAS_LD the argument is CONVERTED DOWN to double, destroying the
 * extended precision before the call.  Measured (gcc 13.3, x86-64):
 *
 *     x = 1 + 1e-17L        (representable in long double, not double)
 *     sqrtl(x) - 1 = 4.98733e-18
 *     sqrt (x) - 1 = 0            <-- all extended precision gone
 *
 * -Wdouble-promotion does NOT diagnose this.
 *-------------------------------------------------------------------*/
#if   defined(NAS_FP32)
#define nas_sqrt   sqrtf
#define nas_fabs   fabsf
#define nas_pow    powf
#define nas_exp    expf
#define nas_log    logf
#define nas_sin    sinf
#define nas_cos    cosf
#define nas_atan   atanf
#define nas_ceil   ceilf
#define nas_floor  floorf
#define nas_fmax   fmaxf
#define nas_fmin   fminf

#elif defined(NAS_LD)
#define nas_sqrt   sqrtl
#define nas_fabs   fabsl
#define nas_pow    powl
#define nas_exp    expl
#define nas_log    logl
#define nas_sin    sinl
#define nas_cos    cosl
#define nas_atan   atanl
#define nas_ceil   ceill
#define nas_floor  floorl
#define nas_fmax   fmaxl
#define nas_fmin   fminl

#else
#define nas_sqrt   sqrt
#define nas_fabs   fabs
#define nas_pow    pow
#define nas_exp    exp
#define nas_log    log
#define nas_sin    sin
#define nas_cos    cos
#define nas_atan   atan
#define nas_ceil   ceil
#define nas_floor  floor
#define nas_fmax   fmax
#define nas_fmin   fmin
#endif

/*---------------------------------------------------------------------
 * Literal suffix.
 *
 * An unsuffixed FP literal is a double.  In a mixed expression the
 * literal wins, so  x * 0.5  is evaluated in DOUBLE even when x is
 * float -- silently restoring the precision the fp32 build is meant to
 * lose.  NAS_R(0.5) casts to the working type instead.
 *
 * Under NAS_LD it also matters in the other direction: 0.1 is a double
 * literal and carries only double precision, so NAS_R(0.1) is still a
 * double-rounded value.  That is unavoidable without editing every
 * literal to have an L suffix, and is harmless for the constants NAS
 * uses (they are reference values, not accumulations).
 *-------------------------------------------------------------------*/
#define NAS_R(x) ((nas_real)(x))

/*---------------------------------------------------------------------
 * printf helper.
 *
 * Passing a long double to a %e/%f/%g conversion is UNDEFINED BEHAVIOUR:
 * varargs does not convert it, and the callee reads 8 bytes where 16 were
 * pushed -- which also misaligns every subsequent argument in the list.
 * Observed in EP under -DNAS_LD: "Sums = 4.902820000000000e+05" against
 * an expected -3.247834652034740e3, and verification failing as a result.
 * A float argument is fine (varargs promotes it to double), so casting to
 * double at the call site is correct and uniform in all three trees.
 *-------------------------------------------------------------------*/
#define NAS_PR(x) ((double)(x))

/*---------------------------------------------------------------------
 * RNG PRECISION BOUNDARY -- DO NOT MAKE THIS FOLLOW THE SWITCH.
 *
 * randlc/vranlc implement a 46-bit linear congruential generator whose
 * multiplier is 1220703125 (= 5^13).  In float that constant rounds to
 * 1220703072, breaking the modular arithmetic and producing a
 * STRUCTURALLY DIFFERENT problem -- a different matrix and right-hand
 * side, not a lower-precision version of the same one.  That would
 * confound the branch comparison completely.
 *
 * The generator therefore stays double in every tree; only the cast of
 * its output into the solver is a precision boundary.  Used by EP and
 * IS only (BT/CG/LU/MG/SP do not call it).
 *-------------------------------------------------------------------*/
typedef double nas_rng_real;

#endif /* NAS_PRECISION_H */
