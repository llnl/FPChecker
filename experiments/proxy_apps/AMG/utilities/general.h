/*BHEADER**********************************************************************
 * Copyright (c) 2017,  Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * Written by Ulrike Yang (yang11@llnl.gov) et al. CODE-LLNL-738-322.
 * This file is part of AMG.  See files README and COPYRIGHT for details.
 *
 * AMG is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License (as published by the Free
 * Software Foundation) version 2.1 dated February 1999.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTIBILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the
 * GNU General Public License for more details.
 *
 ***********************************************************************EHEADER*/

/******************************************************************************
 *
 * General structures and values
 *
 *****************************************************************************/

#ifndef hypre_GENERAL_HEADER
#define hypre_GENERAL_HEADER

/* This allows us to consistently avoid 'int' throughout hypre */
typedef int               hypre_int;
typedef long int          hypre_longint;
typedef unsigned int      hypre_uint;
typedef unsigned long int hypre_ulongint;

/* This allows us to consistently avoid 'double' throughout hypre */
typedef double            hypre_double;

/*--------------------------------------------------------------------------
 * Define various functions
 *--------------------------------------------------------------------------*/

#ifndef hypre_max
#define hypre_max(a,b)  (((a)<(b)) ? (b) : (a))
#endif
#ifndef hypre_min
#define hypre_min(a,b)  (((a)<(b)) ? (a) : (b))
#endif

#ifndef hypre_abs
#define hypre_abs(a)  (((a)>0) ? (a) : -(a))
#endif

#ifndef hypre_round
#define hypre_round(x)  ( ((x) < 0.0) ? ((HYPRE_Int)(x - 0.5)) : ((HYPRE_Int)(x + 0.5)) )
#endif

#ifndef hypre_pow2
#define hypre_pow2(i)  ( 1 << (i) )
#endif

#endif

