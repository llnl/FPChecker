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
 * Header file for HYPRE_utilities library
 *
 *****************************************************************************/
#include "HYPRE.h"

#ifndef HYPRE_UTILITIES_HEADER
#define HYPRE_UTILITIES_HEADER

#ifndef HYPRE_SEQUENTIAL
#include "mpi.h"
#endif

#ifdef HYPRE_USING_OPENMP
#include <omp.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * Before a version of HYPRE goes out the door, increment the version
 * number and check in this file (for CVS to substitute the Date).
 */
#define HYPRE_Version() "HYPRE_RELEASE_NAME Date Compiled: " __DATE__ " " __TIME__

/*--------------------------------------------------------------------------
 * Real and Complex types
 *--------------------------------------------------------------------------*/

#include <float.h>

#if defined(HYPRE_SINGLE)
typedef float HYPRE_Real;
#define HYPRE_REAL_MAX FLT_MAX
#define HYPRE_REAL_MIN FLT_MIN
#define HYPRE_REAL_EPSILON FLT_EPSILON
#define HYPRE_REAL_MIN_EXP FLT_MIN_EXP
#define HYPRE_MPI_REAL MPI_FLOAT

#elif defined(HYPRE_LONG_DOUBLE)
typedef long double HYPRE_Real;
#define HYPRE_REAL_MAX LDBL_MAX
#define HYPRE_REAL_MIN LDBL_MIN
#define HYPRE_REAL_EPSILON LDBL_EPSILON
#define HYPRE_REAL_MIN_EXP DBL_MIN_EXP
#define HYPRE_MPI_REAL MPI_LONG_DOUBLE

#else /* default */
typedef double HYPRE_Real;
#define HYPRE_REAL_MAX DBL_MAX
#define HYPRE_REAL_MIN DBL_MIN
#define HYPRE_REAL_EPSILON DBL_EPSILON
#define HYPRE_REAL_MIN_EXP DBL_MIN_EXP
#define HYPRE_MPI_REAL MPI_DOUBLE
#endif

#if defined(HYPRE_COMPLEX)
typedef double _Complex HYPRE_Complex;
#define HYPRE_MPI_COMPLEX MPI_C_DOUBLE_COMPLEX  /* or MPI_LONG_DOUBLE ? */

#else  /* default */
typedef HYPRE_Real HYPRE_Complex;
#define HYPRE_MPI_COMPLEX HYPRE_MPI_REAL
#endif

/*--------------------------------------------------------------------------
 * Sequential MPI stuff
 *--------------------------------------------------------------------------*/

#ifdef HYPRE_SEQUENTIAL
typedef HYPRE_Int MPI_Comm;
#endif

/*--------------------------------------------------------------------------
 * HYPRE error codes
 *--------------------------------------------------------------------------*/

#define HYPRE_ERROR_GENERIC         1   /* generic error */
#define HYPRE_ERROR_MEMORY          2   /* unable to allocate memory */
#define HYPRE_ERROR_ARG             4   /* argument error */
/* bits 4-8 are reserved for the index of the argument error */
#define HYPRE_ERROR_CONV          256   /* method did not converge as expected */

/*--------------------------------------------------------------------------
 * HYPRE error user functions
 *--------------------------------------------------------------------------*/

/* Return the current hypre error flag */
HYPRE_Int HYPRE_GetError();

/* Check if the given error flag contains the given error code */
HYPRE_Int HYPRE_CheckError(HYPRE_Int hypre_ierr, HYPRE_Int hypre_error_code);

/* Return the index of the argument (counting from 1) where
   argument error (HYPRE_ERROR_ARG) has occured */
HYPRE_Int HYPRE_GetErrorArg();

/* Describe the given error flag in the given string */
void HYPRE_DescribeError(HYPRE_Int hypre_ierr, char *descr);

/* Clears the hypre error flag */
HYPRE_Int HYPRE_ClearAllErrors();

/* Clears the given error code from the hypre error flag */
HYPRE_Int HYPRE_ClearError(HYPRE_Int hypre_error_code);

/*--------------------------------------------------------------------------
 * HYPRE AP user functions
 *--------------------------------------------------------------------------*/

/*Checks whether the AP is on */
HYPRE_Int HYPRE_AssumedPartitionCheck();


#ifdef __cplusplus
}
#endif

#endif
