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
 * Header file for HYPRE library
 *
 *****************************************************************************/

#ifndef HYPRE_HEADER
#define HYPRE_HEADER

#define HYPRE_NO_GLOBAL_PARTITION 1

/*--------------------------------------------------------------------------
 * Type definitions
 *--------------------------------------------------------------------------*/

#ifdef HYPRE_BIGINT
typedef long long int HYPRE_Int;
#define HYPRE_MPI_INT MPI_LONG_LONG
#else
typedef int HYPRE_Int;
#define HYPRE_MPI_INT MPI_INT
#endif

/*--------------------------------------------------------------------------
 * Constants
 *--------------------------------------------------------------------------*/

#define HYPRE_UNITIALIZED -999

#define HYPRE_PETSC_MAT_PARILUT_SOLVER 222
#define HYPRE_PARILUT                  333

#define HYPRE_STRUCT  1111
#define HYPRE_SSTRUCT 3333
#define HYPRE_PARCSR  5555

#define HYPRE_ISIS    9911
#define HYPRE_PETSC   9933

#define HYPRE_PFMG    10
#define HYPRE_SMG     11
#define HYPRE_Jacobi  17

#endif
