#ifndef HYPRE_MATH_PRECISION_H
#define HYPRE_MATH_PRECISION_H
/*--------------------------------------------------------------------------
 * Precision-correct libm dispatch for hypre / AMG.
 *
 * WHY THIS IS NEEDED: hypre's HYPRE_SINGLE / HYPRE_LONG_DOUBLE macros
 * switch the HYPRE_Real typedef, the MPI datatype, and (via
 * hypre_printf.c) the printf conversions -- but NOT the libm calls.  AMG
 * has 255 bare math call sites (160 fabs, 53 sqrt, 41 pow, 1 ceil).
 *
 * C has no overloading, so a bare sqrt() is 'double sqrt(double)':
 *   HYPRE_SINGLE      -> argument promoted, computed in DOUBLE, rounded
 *                        back.  The fp32 build is not really fp32 at
 *                        these sites.
 *   HYPRE_LONG_DOUBLE -> argument CONVERTED DOWN to double.  Extended
 *                        precision is destroyed before the call.
 *
 * Measured, gcc 13.3, x86-64 (80-bit long double):
 *   x = 1 + 1e-17L  (representable in long double, not in double)
 *   sqrtl(x) - 1 = 4.98733e-18
 *   sqrt (x) - 1 = 0            <-- all extended precision gone
 *   sizeof(fabs((long double)x)) = 8
 *
 * That makes an unpatched HYPRE_LONG_DOUBLE build "long double storage,
 * double arithmetic" -- useless as an oracle reference.
 *------------------------------------------------------------------------*/

#include <math.h>

#if defined(HYPRE_SINGLE)
#define hypre_sqrt  sqrtf
#define hypre_fabs  fabsf
#define hypre_pow   powf
#define hypre_ceil  ceilf
#define hypre_floor floorf
#define hypre_log   logf
#define hypre_exp   expf

#elif defined(HYPRE_LONG_DOUBLE)
#define hypre_sqrt  sqrtl
#define hypre_fabs  fabsl
#define hypre_pow   powl
#define hypre_ceil  ceill
#define hypre_floor floorl
#define hypre_log   logl
#define hypre_exp   expl

#else /* default: double */
#define hypre_sqrt  sqrt
#define hypre_fabs  fabs
#define hypre_pow   pow
#define hypre_ceil  ceil
#define hypre_floor floor
#define hypre_log   log
#define hypre_exp   exp
#endif

#endif
