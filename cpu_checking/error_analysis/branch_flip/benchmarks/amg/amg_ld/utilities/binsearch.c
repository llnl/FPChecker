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


 
#include "_hypre_utilities.h"
 
/*--------------------------------------------------------------------------
 * hypre_BinarySearch
 * to contain ordered nonnegative numbers
 * the routine returns the location of the value or -1
 *--------------------------------------------------------------------------*/
 
HYPRE_Int hypre_BinarySearch(HYPRE_Int *list, HYPRE_Int value, HYPRE_Int list_length)
{
   HYPRE_Int low, high, m;
   HYPRE_Int not_found = 1;

   low = 0;
   high = list_length-1; 
   while (not_found && low <= high)
   {
      m = (low + high) / 2;
      if (value < list[m])
      {
         high = m - 1;
      }
      else if (value > list[m])
      {
        low = m + 1;
      }
      else
      {
        not_found = 0;
        return m;
      }
   }
   return -1;
}

/*--------------------------------------------------------------------------
 * hypre_BinarySearch2
 * this one is a bit more robust:
 *   avoids overflow of m as can happen above when (low+high) overflows
 *   lets user specifiy high and low bounds for array (so a subset 
     of array can be used)
 *  if not found, then spot returns where is should be inserted

 *--------------------------------------------------------------------------*/
 
HYPRE_Int hypre_BinarySearch2(HYPRE_Int *list, HYPRE_Int value, HYPRE_Int low, HYPRE_Int high, HYPRE_Int *spot) 
{
   
   HYPRE_Int m;
   
   while (low <= high) 
   {
      m = low + (high - low)/2;
 
      if (value < list[m])
         high = m - 1;
      else if (value > list[m])
         low = m + 1;
      else
      {
         *spot = m;
         return m;
      }
   }
   /* not found (high = low-1) - so insert at low */
      *spot = low;

   return -1;
}

/*--------------------------------------------------------------------------
 * Equivalent to C++ std::lower_bound
 *--------------------------------------------------------------------------*/
 
HYPRE_Int *hypre_LowerBound( HYPRE_Int *first, HYPRE_Int *last, HYPRE_Int value )
{
   HYPRE_Int *it;
   size_t count = last - first, step;

   while (count > 0) {
      it = first; step = count/2; it += step;
      if (*it < value) {
         first = ++it;
         count -= step + 1;
      }
      else count = step;
   }
   return first;
}
