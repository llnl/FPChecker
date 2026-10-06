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

#ifndef hypre_NEW_COMMPKG
#define hypre_NEW_COMMPKG

typedef struct
{
   HYPRE_Int       length;
   HYPRE_Int       storage_length; 
   HYPRE_Int      *id;
   HYPRE_Int      *vec_starts;
   HYPRE_Int       element_storage_length; 
   HYPRE_Int      *elements;
   HYPRE_Real     *d_elements; /* Is this used anywhere? */
   void           *v_elements;
   
}  hypre_ProcListElements;   

#endif /* hypre_NEW_COMMPKG */

