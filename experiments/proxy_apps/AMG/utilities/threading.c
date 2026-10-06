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

#include <stdlib.h>
#include <stdio.h>
#include "_hypre_utilities.h"

#ifdef HYPRE_USING_OPENMP

HYPRE_Int
hypre_NumThreads( )
{
   HYPRE_Int num_threads;

   num_threads = omp_get_max_threads();

   return num_threads;
}

/* This next function must be called from within a parallel region! */

HYPRE_Int
hypre_NumActiveThreads( )
{
   HYPRE_Int num_threads;

   num_threads = omp_get_num_threads();

   return num_threads;
}

/* This next function must be called from within a parallel region! */

HYPRE_Int
hypre_GetThreadNum( )
{
   HYPRE_Int my_thread_num;

   my_thread_num = omp_get_thread_num();

   return my_thread_num;
}

#endif

/* This next function must be called from within a parallel region! */

void
hypre_GetSimpleThreadPartition( HYPRE_Int *begin, HYPRE_Int *end, HYPRE_Int n )
{
   HYPRE_Int num_threads = hypre_NumActiveThreads();
   HYPRE_Int my_thread_num = hypre_GetThreadNum();

   HYPRE_Int n_per_thread = (n + num_threads - 1)/num_threads;

   *begin = hypre_min(n_per_thread*my_thread_num, n);
   *end = hypre_min(*begin + n_per_thread, n);
}
