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
 * Relaxation scheme
 *
 *****************************************************************************/

#include "_hypre_parcsr_ls.h"


/*--------------------------------------------------------------------------
 * hypre_BoomerAMGRelax
 *--------------------------------------------------------------------------*/

HYPRE_Int  hypre_BoomerAMGRelaxIF( hypre_ParCSRMatrix *A,
                             hypre_ParVector    *f,
                             HYPRE_Int                *cf_marker,
                             HYPRE_Int                 relax_type,
                             HYPRE_Int                 relax_order,
                             HYPRE_Int                 cycle_type,
                             HYPRE_Real          relax_weight,
                             HYPRE_Real          omega,
                             HYPRE_Real         *l1_norms,
                             hypre_ParVector    *u,
                             hypre_ParVector    *Vtemp,
                             hypre_ParVector    *Ztemp )
{
   HYPRE_Int i, Solve_err_flag = 0;
   HYPRE_Int relax_points[2];
   if (relax_order == 1 && cycle_type < 3)
   {
      if (cycle_type < 2)
      {
         relax_points[0] = 1;
	 relax_points[1] = -1;
      }
      else
      {
	 relax_points[0] = -1;
	 relax_points[1] = 1;
      }

      for (i=0; i < 2; i++)
         Solve_err_flag = hypre_BoomerAMGRelax(A,
                                               f,
                                               cf_marker,
                                               relax_type,
                                               relax_points[i],
                                               relax_weight,
                                               omega,
                                               l1_norms,
                                               u,
                                               Vtemp, 
                                               Ztemp); 
      
   }
   else
   {
      Solve_err_flag = hypre_BoomerAMGRelax(A,
                                            f,
                                            cf_marker,
                                            relax_type,
                                            0,
                                            relax_weight,
                                            omega,
                                            l1_norms,
                                            u,
                                            Vtemp, 
                                            Ztemp); 
   }

   return Solve_err_flag;
}


