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

#include "_hypre_parcsr_ls.h"
#include "par_amg.h"

#define DEBUG 0
#define PRINT_CF 0

/*****************************************************************************
 *
 * Routine for driving the setup phase of AMG
 *
 *****************************************************************************/

/*****************************************************************************
 * hypre_BoomerAMGSetup
 *****************************************************************************/

HYPRE_Int
hypre_BoomerAMGSetup( void               *amg_vdata,
                   hypre_ParCSRMatrix *A,
                   hypre_ParVector    *f,
                   hypre_ParVector    *u         )
{
   MPI_Comm 	      comm = hypre_ParCSRMatrixComm(A); 
   hypre_ParAMGData   *amg_data = (hypre_ParAMGData*) amg_vdata;

   /* Data Structure variables */

   hypre_ParCSRMatrix **A_array;
   hypre_ParVector    **F_array;
   hypre_ParVector    **U_array;
   hypre_ParVector     *Vtemp = NULL;
   hypre_ParVector     *Rtemp = NULL;
   hypre_ParVector     *Ptemp = NULL;
   hypre_ParVector     *Ztemp = NULL;
   hypre_ParCSRMatrix **P_array;
   hypre_ParVector    *Residual_array;
   HYPRE_Int                **CF_marker_array;   
   HYPRE_Int                **dof_func_array;   
   HYPRE_Int                 *dof_func;
   HYPRE_Int                 *col_offd_S_to_A;
   HYPRE_Int                 *col_offd_SN_to_AN;
   HYPRE_Real          *relax_weight;
   HYPRE_Real          *omega;
   HYPRE_Real           schwarz_relax_wt = 1;
   HYPRE_Real           strong_threshold;
   HYPRE_Real           max_row_sum;
   HYPRE_Real           trunc_factor;
   HYPRE_Real           agg_trunc_factor, agg_P12_trunc_factor;
   HYPRE_Real           S_commpkg_switch;
   HYPRE_Int       relax_order;
   HYPRE_Int      max_levels; 
   HYPRE_Int      amg_logging;
   HYPRE_Int      amg_print_level;
   HYPRE_Int      debug_flag;
   HYPRE_Int      dbg_flg;
   HYPRE_Int      local_num_vars;
   HYPRE_Int      P_max_elmts;
   HYPRE_Int      agg_P_max_elmts;
   HYPRE_Int      agg_P12_max_elmts;
   HYPRE_Int      mult_additive = hypre_ParAMGDataMultAdditive(amg_data);
   HYPRE_Int      additive = hypre_ParAMGDataAdditive(amg_data);
   HYPRE_Int      simple = hypre_ParAMGDataSimple(amg_data);
   HYPRE_Int      add_last_lvl = hypre_ParAMGDataAddLastLvl(amg_data);
   HYPRE_Int      add_P_max_elmts = hypre_ParAMGDataMultAddPMaxElmts(amg_data);
   HYPRE_Real     add_trunc_factor = hypre_ParAMGDataMultAddTruncFactor(amg_data);
   HYPRE_Int      add_rlx = hypre_ParAMGDataAddRelaxType(amg_data);
   HYPRE_Real     add_rlx_wt = hypre_ParAMGDataAddRelaxWt(amg_data);

   /* Local variables */
   HYPRE_Int                 *CF_marker;
   HYPRE_Int                 *CFN_marker;
   HYPRE_Int                 *CF2_marker;
   hypre_ParCSRMatrix  *S = NULL;
   hypre_ParCSRMatrix  *S2;
   hypre_ParCSRMatrix  *SN = NULL;
   hypre_ParCSRMatrix  *SCR;
   hypre_ParCSRMatrix  *P = NULL;
   hypre_ParCSRMatrix  *A_H;
   hypre_ParCSRMatrix  *AN = NULL;
   hypre_ParCSRMatrix  *P1;
   hypre_ParCSRMatrix  *P2;
   hypre_ParCSRMatrix  *Pnew = NULL;
   HYPRE_Real          *SmoothVecs = NULL;
   HYPRE_Real         **l1_norms = NULL;
   HYPRE_Real         **cheby_ds = NULL;
   HYPRE_Real         **cheby_coefs = NULL;

   HYPRE_Int       old_num_levels, num_levels;
   HYPRE_Int       level;
   HYPRE_Int       local_size, i;
   HYPRE_Int       first_local_row;
   HYPRE_Int       coarse_size;
   HYPRE_Int       coarsen_type;
   HYPRE_Int       measure_type;
   HYPRE_Int       setup_type;
   HYPRE_Int       fine_size;
   HYPRE_Int       rest, tms, indx;
   HYPRE_Real    size;
   HYPRE_Int       not_finished_coarsening = 1;
   HYPRE_Int       coarse_threshold = hypre_ParAMGDataMaxCoarseSize(amg_data);
   HYPRE_Int       min_coarse_size = hypre_ParAMGDataMinCoarseSize(amg_data);
   HYPRE_Int       seq_threshold = hypre_ParAMGDataSeqThreshold(amg_data);
   HYPRE_Int       j, k;
   HYPRE_Int       num_procs,my_id,num_threads;
   HYPRE_Int      *grid_relax_type = hypre_ParAMGDataGridRelaxType(amg_data);
   HYPRE_Int       num_functions = hypre_ParAMGDataNumFunctions(amg_data);
   HYPRE_Int       nodal = hypre_ParAMGDataNodal(amg_data);
   HYPRE_Int       num_paths = hypre_ParAMGDataNumPaths(amg_data);
   HYPRE_Int       agg_num_levels = hypre_ParAMGDataAggNumLevels(amg_data);
   HYPRE_Int       agg_interp_type = hypre_ParAMGDataAggInterpType(amg_data);
   HYPRE_Int       sep_weight = hypre_ParAMGDataSepWeight(amg_data); 
   HYPRE_Int	    *coarse_dof_func = NULL;
   HYPRE_Int	    *coarse_pnts_global;
   HYPRE_Int	    *coarse_pnts_global1;
   HYPRE_Int       num_cg_sweeps;

   HYPRE_Real *max_eig_est = NULL;
   HYPRE_Real *min_eig_est = NULL;

   HYPRE_Int interp_type;

   /* parameters for non-Galerkin stuff */
   HYPRE_Int nongalerk_num_tol = hypre_ParAMGDataNonGalerkNumTol (amg_data);
   HYPRE_Real *nongalerk_tol = hypre_ParAMGDataNonGalerkTol (amg_data); 
   HYPRE_Real nongalerk_tol_l = 0.0; 
   HYPRE_Real *nongal_tol_array = hypre_ParAMGDataNonGalTolArray (amg_data); 

   HYPRE_Int block_mode = 0;

   HYPRE_Int mult_addlvl = hypre_max(mult_additive, simple);
   HYPRE_Int addlvl = hypre_max(mult_addlvl, additive);
   HYPRE_Int rap2 = hypre_ParAMGDataRAP2(amg_data);
   HYPRE_Int keepTranspose = hypre_ParAMGDataKeepTranspose(amg_data);

   HYPRE_Int *num_grid_sweeps = hypre_ParAMGDataNumGridSweeps(amg_data);
   HYPRE_Int ns = num_grid_sweeps[1];
   HYPRE_Real    wall_time;   /* for debugging instrumentation */
   HYPRE_Int      add_end;
   HYPRE_Real    cum_nnz_AP;

   hypre_MPI_Comm_size(comm, &num_procs);   
   hypre_MPI_Comm_rank(comm,&my_id);

   num_threads = hypre_NumThreads();

   old_num_levels = hypre_ParAMGDataNumLevels(amg_data);
   max_levels = hypre_ParAMGDataMaxLevels(amg_data);
   add_end = hypre_min(add_last_lvl, max_levels-1);
   if (add_end == -1) add_end = max_levels-1;
   amg_logging = hypre_ParAMGDataLogging(amg_data);
   amg_print_level = hypre_ParAMGDataPrintLevel(amg_data);
   coarsen_type = hypre_ParAMGDataCoarsenType(amg_data);
   measure_type = hypre_ParAMGDataMeasureType(amg_data);
   setup_type = hypre_ParAMGDataSetupType(amg_data);
   debug_flag = hypre_ParAMGDataDebugFlag(amg_data);
   relax_weight = hypre_ParAMGDataRelaxWeight(amg_data);
   omega = hypre_ParAMGDataOmega(amg_data);
   dof_func = hypre_ParAMGDataDofFunc(amg_data);
   interp_type = hypre_ParAMGDataInterpType(amg_data);

   relax_order         = hypre_ParAMGDataRelaxOrder(amg_data);

   hypre_ParCSRMatrixSetNumNonzeros(A);
   hypre_ParCSRMatrixSetDNumNonzeros(A);
   hypre_ParAMGDataNumVariables(amg_data) = hypre_ParCSRMatrixNumRows(A);

   if (num_procs == 1) seq_threshold = 0;
   if (setup_type == 0) return hypre_error_flag;

   S = NULL;

   A_array = hypre_ParAMGDataAArray(amg_data);
   P_array = hypre_ParAMGDataPArray(amg_data);
   CF_marker_array = hypre_ParAMGDataCFMarkerArray(amg_data);
   dof_func_array = hypre_ParAMGDataDofFuncArray(amg_data);
   local_size = hypre_CSRMatrixNumRows(hypre_ParCSRMatrixDiag(A));

   grid_relax_type[3] = hypre_ParAMGDataUserCoarseRelaxType(amg_data); 

   /* change in definition of standard and multipass interpolation, by
      eliminating interp_type 9 and 5 and setting sep_weight instead
      when using separation of weights option */
   if (interp_type == 9)
   {
      interp_type = 8;
      sep_weight = 1;
   }
   else if (interp_type == 5)
   {
      interp_type = 4;
      sep_weight = 1;
   }

   /* free up storage in case of new setup without prvious destroy */

   if (A_array || P_array || CF_marker_array || dof_func_array)
   {
      for (j = 1; j < old_num_levels; j++)
      {
         if (A_array[j])
         {
            hypre_ParCSRMatrixDestroy(A_array[j]);
            A_array[j] = NULL;
         }

         if (dof_func_array[j])
         {
            hypre_TFree(dof_func_array[j]);
            dof_func_array[j] = NULL;
         }
      }

      for (j = 0; j < old_num_levels-1; j++)
      {
         if (P_array[j])
         {
            hypre_ParCSRMatrixDestroy(P_array[j]);
            P_array[j] = NULL;
         }
      }

/* Special case use of CF_marker_array when old_num_levels == 1
   requires us to attempt this deallocation every time */
      if (CF_marker_array[0])
      {
        hypre_TFree(CF_marker_array[0]);
        CF_marker_array[0] = NULL;
      }

      for (j = 1; j < old_num_levels-1; j++)
      {
         if (CF_marker_array[j])
         {
            hypre_TFree(CF_marker_array[j]);
            CF_marker_array[j] = NULL;
         }
      }
   }

   {
      MPI_Comm new_comm = hypre_ParAMGDataNewComm(amg_data);
      void *amg = hypre_ParAMGDataCoarseSolver(amg_data);
      if (hypre_ParAMGDataRtemp(amg_data))
      {
         hypre_ParVectorDestroy(hypre_ParAMGDataRtemp(amg_data));
         hypre_ParAMGDataRtemp(amg_data) = NULL;
      }
      if (hypre_ParAMGDataPtemp(amg_data))
      {
         hypre_ParVectorDestroy(hypre_ParAMGDataPtemp(amg_data));
         hypre_ParAMGDataPtemp(amg_data) = NULL;
      }
      if (hypre_ParAMGDataZtemp(amg_data))
      {
         hypre_ParVectorDestroy(hypre_ParAMGDataZtemp(amg_data));
         hypre_ParAMGDataZtemp(amg_data) = NULL;
      }
   
      if (hypre_ParAMGDataACoarse(amg_data))
      {
         hypre_ParCSRMatrixDestroy(hypre_ParAMGDataACoarse(amg_data));
         hypre_ParAMGDataACoarse(amg_data) = NULL;
      }
   
      if (hypre_ParAMGDataUCoarse(amg_data))
      {
         hypre_ParVectorDestroy(hypre_ParAMGDataUCoarse(amg_data));
         hypre_ParAMGDataUCoarse(amg_data) = NULL;
      }
   
      if (hypre_ParAMGDataFCoarse(amg_data))
      {
         hypre_ParVectorDestroy(hypre_ParAMGDataFCoarse(amg_data));
         hypre_ParAMGDataFCoarse(amg_data) = NULL;
      }
   
      if (hypre_ParAMGDataAMat(amg_data)) 
      {
	 hypre_TFree(hypre_ParAMGDataAMat(amg_data));
	 hypre_ParAMGDataAMat(amg_data) = NULL;
      }
      if (hypre_ParAMGDataBVec(amg_data)) 
      {
	 hypre_TFree(hypre_ParAMGDataBVec(amg_data));
	 hypre_ParAMGDataBVec(amg_data) = NULL;
      }
      if (hypre_ParAMGDataCommInfo(amg_data)) 
      {
	 hypre_TFree(hypre_ParAMGDataCommInfo(amg_data));
	 hypre_ParAMGDataCommInfo(amg_data) = NULL;
      }
   
      if (new_comm != hypre_MPI_COMM_NULL)
      {
         hypre_MPI_Comm_free (&new_comm);
         hypre_ParAMGDataNewComm(amg_data) = hypre_MPI_COMM_NULL;
      }
  
      if (amg)
      {
         hypre_BoomerAMGDestroy (amg);
         hypre_ParAMGDataCoarseSolver(amg_data) = NULL;
      }

      if (hypre_ParAMGDataMaxEigEst(amg_data))
      {
         hypre_TFree(hypre_ParAMGDataMaxEigEst(amg_data));
         hypre_ParAMGDataMaxEigEst(amg_data) = NULL;
      }
      if (hypre_ParAMGDataMinEigEst(amg_data))
      {
         hypre_TFree(hypre_ParAMGDataMinEigEst(amg_data));
         hypre_ParAMGDataMinEigEst(amg_data) = NULL;
      }
      if (hypre_ParAMGDataL1Norms(amg_data))
      {
         for (i=0; i < old_num_levels; i++)
            if (hypre_ParAMGDataL1Norms(amg_data)[i])
              hypre_TFree(hypre_ParAMGDataL1Norms(amg_data)[i]);
         hypre_TFree(hypre_ParAMGDataL1Norms(amg_data));
      }
      if ( hypre_ParAMGDataResidual(amg_data) ) {
         hypre_ParVectorDestroy( hypre_ParAMGDataResidual(amg_data) );
         hypre_ParAMGDataResidual(amg_data) = NULL;
      }
   }

   if (A_array == NULL)
      A_array = hypre_CTAlloc(hypre_ParCSRMatrix*, max_levels);

   if (P_array == NULL && max_levels > 1)
      P_array = hypre_CTAlloc(hypre_ParCSRMatrix*, max_levels-1);

   if (CF_marker_array == NULL)
      CF_marker_array = hypre_CTAlloc(HYPRE_Int*, max_levels);
   if (dof_func_array == NULL)
      dof_func_array = hypre_CTAlloc(HYPRE_Int*, max_levels);
   if (num_functions > 1 && dof_func == NULL)
   {
      first_local_row = hypre_ParCSRMatrixFirstRowIndex(A);
      dof_func = hypre_CTAlloc(HYPRE_Int,local_size);
      rest = first_local_row-((first_local_row/num_functions)*num_functions);
      indx = num_functions-rest;
      if (rest == 0) indx = 0;
      k = num_functions - 1;
      for (j = indx-1; j > -1; j--)
         dof_func[j] = k--;
      tms = local_size/num_functions;
      if (tms*num_functions+indx > local_size) tms--;
      for (j=0; j < tms; j++)
      {
         for (k=0; k < num_functions; k++)
            dof_func[indx++] = k;
      }
      k = 0;
      while (indx < local_size)
         dof_func[indx++] = k++;
      hypre_ParAMGDataDofFunc(amg_data) = dof_func;
   }

   A_array[0] = A;
                                                                             
   dof_func_array[0] = dof_func;
   hypre_ParAMGDataCFMarkerArray(amg_data) = CF_marker_array;
   hypre_ParAMGDataDofFuncArray(amg_data) = dof_func_array;
   hypre_ParAMGDataAArray(amg_data) = A_array;
   hypre_ParAMGDataPArray(amg_data) = P_array;
   hypre_ParAMGDataRArray(amg_data) = P_array;

   Vtemp = hypre_ParAMGDataVtemp(amg_data);

   if (Vtemp != NULL)
   {
      hypre_ParVectorDestroy(Vtemp);
      Vtemp = NULL;
   }

   Vtemp = hypre_ParVectorCreate(hypre_ParCSRMatrixComm(A_array[0]),
                                 hypre_ParCSRMatrixGlobalNumRows(A_array[0]),
                                 hypre_ParCSRMatrixRowStarts(A_array[0]));
   hypre_ParVectorInitialize(Vtemp);
   hypre_ParVectorSetPartitioningOwner(Vtemp,0);
   hypre_ParAMGDataVtemp(amg_data) = Vtemp;

   if (relax_weight[0] < 0 || omega[0] < 0)
   {
      Ptemp = hypre_ParVectorCreate(hypre_ParCSRMatrixComm(A_array[0]),
                                 hypre_ParCSRMatrixGlobalNumRows(A_array[0]),
                                 hypre_ParCSRMatrixRowStarts(A_array[0]));
      hypre_ParVectorInitialize(Ptemp);
      hypre_ParVectorSetPartitioningOwner(Ptemp,0);
      hypre_ParAMGDataPtemp(amg_data) = Ptemp;
      Rtemp = hypre_ParVectorCreate(hypre_ParCSRMatrixComm(A_array[0]),
                                 hypre_ParCSRMatrixGlobalNumRows(A_array[0]),
                                 hypre_ParCSRMatrixRowStarts(A_array[0]));
      hypre_ParVectorInitialize(Rtemp);
      hypre_ParVectorSetPartitioningOwner(Rtemp,0);
      hypre_ParAMGDataRtemp(amg_data) = Rtemp;
   }
  
   /* See if we need the Ztemp vector */
   if (relax_weight[0] < 0 || omega[0] < 0)
   {
      Ztemp = hypre_ParVectorCreate(hypre_ParCSRMatrixComm(A_array[0]),
                                    hypre_ParCSRMatrixGlobalNumRows(A_array[0]),
                                    hypre_ParCSRMatrixRowStarts(A_array[0]));
      hypre_ParVectorInitialize(Ztemp);
      hypre_ParVectorSetPartitioningOwner(Ztemp,0);
      hypre_ParAMGDataZtemp(amg_data) = Ztemp;
   }
   else if (grid_relax_type[0] == 16 || grid_relax_type[1] == 16 || grid_relax_type[2] == 16 || grid_relax_type[3] == 16)
   {
      /* Chebyshev */
       Ztemp = hypre_ParVectorCreate(hypre_ParCSRMatrixComm(A_array[0]),
                                     hypre_ParCSRMatrixGlobalNumRows(A_array[0]),
                                     hypre_ParCSRMatrixRowStarts(A_array[0]));
       hypre_ParVectorInitialize(Ztemp);
       hypre_ParVectorSetPartitioningOwner(Ztemp,0);
       hypre_ParAMGDataZtemp(amg_data) = Ztemp;
      
   }
   else if (num_threads > 1)
   {
      /* we need the temp Z vector for relaxation 3 and 6 now if we are
       * using threading */
      for (j = 1; j < 4; j++)
      {
         if (grid_relax_type[j] == 3 || grid_relax_type[j] == 4 || grid_relax_type[j] == 6  ||
             grid_relax_type[j] == 8 || grid_relax_type[j] == 13 || grid_relax_type[j] == 14)
         {
            Ztemp = hypre_ParVectorCreate(hypre_ParCSRMatrixComm(A_array[0]),
                                          hypre_ParCSRMatrixGlobalNumRows(A_array[0]),
                                          hypre_ParCSRMatrixRowStarts(A_array[0]));
            hypre_ParVectorInitialize(Ztemp);
            hypre_ParVectorSetPartitioningOwner(Ztemp,0);
            hypre_ParAMGDataZtemp(amg_data) = Ztemp;
            break;
         }
      }
   }
   


   F_array = hypre_ParAMGDataFArray(amg_data);
   U_array = hypre_ParAMGDataUArray(amg_data);

   if (F_array != NULL || U_array != NULL)
   {
      for (j = 1; j < old_num_levels; j++)
      {
         if (F_array[j] != NULL)
         {
            hypre_ParVectorDestroy(F_array[j]);
            F_array[j] = NULL;
         }
         if (U_array[j] != NULL)
         {
            hypre_ParVectorDestroy(U_array[j]);
            U_array[j] = NULL;
         }
      }
   }

   if (F_array == NULL)
      F_array = hypre_CTAlloc(hypre_ParVector*, max_levels);
   if (U_array == NULL)
      U_array = hypre_CTAlloc(hypre_ParVector*, max_levels);

   F_array[0] = f;
   U_array[0] = u;

   hypre_ParAMGDataFArray(amg_data) = F_array;
   hypre_ParAMGDataUArray(amg_data) = U_array;

   /*----------------------------------------------------------
    * Initialize hypre_ParAMGData
    *----------------------------------------------------------*/

   not_finished_coarsening = 1;
   level = 0;
  
   strong_threshold = hypre_ParAMGDataStrongThreshold(amg_data);
   max_row_sum = hypre_ParAMGDataMaxRowSum(amg_data);
   trunc_factor = hypre_ParAMGDataTruncFactor(amg_data);
   agg_trunc_factor = hypre_ParAMGDataAggTruncFactor(amg_data);
   agg_P12_trunc_factor = hypre_ParAMGDataAggP12TruncFactor(amg_data);
   P_max_elmts = hypre_ParAMGDataPMaxElmts(amg_data);
   agg_P_max_elmts = hypre_ParAMGDataAggPMaxElmts(amg_data);
   agg_P12_max_elmts = hypre_ParAMGDataAggP12MaxElmts(amg_data);
   S_commpkg_switch = hypre_ParAMGDataSCommPkgSwitch(amg_data);

   /*-----------------------------------------------------
    *  Enter Coarsening Loop
    *-----------------------------------------------------*/

   while (not_finished_coarsening)
   {

      fine_size = hypre_ParCSRMatrixGlobalNumRows(A_array[level]);

      if (level > 0)
      {   

         if (block_mode == 0)
         {
            F_array[level] =
               hypre_ParVectorCreate(hypre_ParCSRMatrixComm(A_array[level]),
                                     hypre_ParCSRMatrixGlobalNumRows(A_array[level]),
                                     hypre_ParCSRMatrixRowStarts(A_array[level]));
            hypre_ParVectorInitialize(F_array[level]);
            hypre_ParVectorSetPartitioningOwner(F_array[level],0);
            
            U_array[level] =
               hypre_ParVectorCreate(hypre_ParCSRMatrixComm(A_array[level]),
                                     hypre_ParCSRMatrixGlobalNumRows(A_array[level]),
                                     hypre_ParCSRMatrixRowStarts(A_array[level]));
            hypre_ParVectorInitialize(U_array[level]);
            hypre_ParVectorSetPartitioningOwner(U_array[level],0);
         }
         
      }

      /*-------------------------------------------------------------
       * Select coarse-grid points on 'level' : returns CF_marker
       * for the level.  Returns strength matrix, S  
       *--------------------------------------------------------------*/
     
      if (debug_flag==1) wall_time = time_getWallclockSeconds();
      if (debug_flag==3)
      {
          hypre_printf("\n ===== Proc = %d     Level = %d  =====\n",
                        my_id, level);
          fflush(NULL);
      }

      if ( max_levels == 1)
      {
	 S = NULL;
	 coarse_pnts_global = NULL;
         CF_marker = hypre_CTAlloc(HYPRE_Int, local_size );
	 for (i=0; i < local_size ; i++)
	    CF_marker[i] = 1;
         /* AB removed below - already allocated */
         /* CF_marker_array = hypre_CTAlloc(HYPRE_Int*, 1);*/
	 CF_marker_array[level] = CF_marker;
	 coarse_size = fine_size;
      }
      else /* max_levels > 1 */
      {
         if (block_mode == 0)
         {
            local_num_vars =
               hypre_CSRMatrixNumRows(hypre_ParCSRMatrixDiag(A_array[level]));
         }

         /**** Get the Strength Matrix ****/        

	 if (nodal == 0)
	 {
	    hypre_BoomerAMGCreateS(A_array[level], 
				   strong_threshold, max_row_sum, 
				   num_functions, dof_func_array[level],&S);
	    col_offd_S_to_A = NULL;
	    if (strong_threshold > S_commpkg_switch)
                  hypre_BoomerAMGCreateSCommPkg(A_array[level],S,
				&col_offd_S_to_A);
	 }


         /**** Do the appropriate coarsening ****/ 

         if (nodal == 0) /* no nodal coarsening */
         { 
           if (coarsen_type == 6)
               hypre_BoomerAMGCoarsenFalgout(S, A_array[level], measure_type,
                                             debug_flag, &CF_marker);
           else if (coarsen_type == 7)
               hypre_BoomerAMGCoarsen(S, A_array[level], 2,
                                      debug_flag, &CF_marker);
           else if (coarsen_type == 8)
               hypre_BoomerAMGCoarsenPMIS(S, A_array[level], 0,
                                          debug_flag, &CF_marker);
           else if (coarsen_type == 9)
               hypre_BoomerAMGCoarsenPMIS(S, A_array[level], 2,
                                          debug_flag, &CF_marker);
           else if (coarsen_type == 10)
               hypre_BoomerAMGCoarsenHMIS(S, A_array[level], measure_type,
                                          debug_flag, &CF_marker);
           else if (coarsen_type)
                  hypre_BoomerAMGCoarsenRuge(S, A_array[level],
                        measure_type, coarsen_type, debug_flag, &CF_marker);
           else
                  hypre_BoomerAMGCoarsen(S, A_array[level], 0,
                                      debug_flag, &CF_marker);
           if (level < agg_num_levels)
           {
               hypre_BoomerAMGCoarseParms(comm, local_num_vars,
                        1, dof_func_array[level], CF_marker,
                        &coarse_dof_func,&coarse_pnts_global1);
               hypre_BoomerAMGCreate2ndS (S, CF_marker, num_paths,
                                       coarse_pnts_global1, &S2);
               if (coarsen_type == 10)
                  hypre_BoomerAMGCoarsenHMIS(S2, S2, measure_type+3,
                                     debug_flag, &CFN_marker);
               else if (coarsen_type == 8)
                   hypre_BoomerAMGCoarsenPMIS(S2, S2, 3,
                                     debug_flag, &CFN_marker);
               else if (coarsen_type == 9)
                   hypre_BoomerAMGCoarsenPMIS(S2, S2, 4,
                                     debug_flag, &CFN_marker);
               else if (coarsen_type == 6)
                  hypre_BoomerAMGCoarsenFalgout(S2, S2, measure_type,
                                       debug_flag, &CFN_marker);
               else if (coarsen_type == 7)
                  hypre_BoomerAMGCoarsen(S2, S2, 2, debug_flag, &CFN_marker);
               else if (coarsen_type)
                  hypre_BoomerAMGCoarsenRuge(S2, S2, measure_type, coarsen_type, 
				debug_flag, &CFN_marker);
               else 
                  hypre_BoomerAMGCoarsen(S2, S2, 0, debug_flag, &CFN_marker);
               hypre_ParCSRMatrixDestroy(S2);
           }
         }
   /*****xxxxxxxxxxxxx changes for min_coarse_size */
         /* here we will determine the coarse grid size to be able to determine if it is not smaller 
	    than requested minimal size */
         if (level >= agg_num_levels)
         {
          if (block_mode == 0 )
          {
            hypre_BoomerAMGCoarseParms(comm, local_num_vars,
                                       num_functions, dof_func_array[level], CF_marker,
                                       &coarse_dof_func,&coarse_pnts_global);
          }
#ifdef HYPRE_NO_GLOBAL_PARTITION
          if (my_id == (num_procs -1)) coarse_size = coarse_pnts_global[1];
          hypre_MPI_Bcast(&coarse_size, 1, HYPRE_MPI_INT, num_procs-1, comm);
#else
          coarse_size = coarse_pnts_global[num_procs];
#endif
          /* if no coarse-grid, stop coarsening, and set the
           * coarsest solve to be a single sweep of default smoother or smoother set by user */
          if ((coarse_size == 0) || (coarse_size == fine_size))
          {
             HYPRE_Int *num_grid_sweeps = hypre_ParAMGDataNumGridSweeps(amg_data);
             HYPRE_Int **grid_relax_points = hypre_ParAMGDataGridRelaxPoints(amg_data);
             if (grid_relax_type[3] == 9 || grid_relax_type[3] == 99
                 || grid_relax_type[3] == 19 || grid_relax_type[3] == 98)
	     {
	        grid_relax_type[3] = grid_relax_type[0];
	        num_grid_sweeps[3] = 1;
	        if (grid_relax_points) grid_relax_points[3][0] = 0; 
	     }
	     if (S) hypre_ParCSRMatrixDestroy(S);
	     if (SN) hypre_ParCSRMatrixDestroy(SN);
	     if (AN) hypre_ParCSRMatrixDestroy(AN);
             hypre_TFree(CF_marker);
             hypre_TFree(coarse_pnts_global);
             if (level > 0)
             {
                /* note special case treatment of CF_marker is necessary
                 * to do CF relaxation correctly when num_levels = 1 */
                hypre_TFree(CF_marker_array[level]);
                hypre_ParVectorDestroy(F_array[level]);
                hypre_ParVectorDestroy(U_array[level]);
             }
             coarse_size = fine_size;
             break; 
          }

          if (coarse_size < min_coarse_size)
          {
	    if (S) hypre_ParCSRMatrixDestroy(S);
	    if (SN) hypre_ParCSRMatrixDestroy(SN);
	    if (AN) hypre_ParCSRMatrixDestroy(AN);
	    if (num_functions > 1) hypre_TFree(coarse_dof_func);
	    hypre_TFree(CF_marker);
            hypre_TFree(coarse_pnts_global);
            if (level > 0)
            {
               hypre_ParVectorDestroy(F_array[level]);
               hypre_ParVectorDestroy(U_array[level]);
            }
            coarse_size = fine_size;
            break; 
          }
         }

   /*****xxxxxxxxxxxxx changes for min_coarse_size  end */
         if (level < agg_num_levels)
         {
            if (nodal == 0)
            {
	       if (agg_interp_type == 1)
          	  hypre_BoomerAMGBuildExtPIInterp(A_array[level], 
		        CF_marker, S, coarse_pnts_global1, 
			num_functions, dof_func_array[level], debug_flag, 
			agg_P12_trunc_factor, agg_P12_max_elmts, col_offd_S_to_A, &P1);
	       else if (agg_interp_type == 2)
                  hypre_BoomerAMGBuildStdInterp(A_array[level], 
		        CF_marker, S, coarse_pnts_global1, 
			num_functions, dof_func_array[level], debug_flag, 
			agg_P12_trunc_factor, agg_P12_max_elmts, 0, col_offd_S_to_A, &P1);
	       else if (agg_interp_type == 3)
                  hypre_BoomerAMGBuildExtInterp(A_array[level], 
		        CF_marker, S, coarse_pnts_global1, 
			num_functions, dof_func_array[level], debug_flag, 
			agg_P12_trunc_factor, agg_P12_max_elmts, col_offd_S_to_A, &P1);
               if (agg_interp_type == 4)
               {
                  hypre_BoomerAMGCorrectCFMarker (CF_marker, local_num_vars, 
			CFN_marker);
                  hypre_TFree(coarse_pnts_global1);
                  /*hypre_TFree(coarse_dof_func);
                  coarse_dof_func = NULL;*/
                  hypre_TFree(CFN_marker);
                  hypre_BoomerAMGCoarseParms(comm, local_num_vars,
                        num_functions, dof_func_array[level], CF_marker,
                        &coarse_dof_func,&coarse_pnts_global); 
                  hypre_BoomerAMGBuildMultipass(A_array[level], 
			CF_marker, S, coarse_pnts_global, 
			num_functions, dof_func_array[level], debug_flag, 
			agg_trunc_factor, agg_P_max_elmts, sep_weight, 
			col_offd_S_to_A, &P);
               }
               else
               {
                  hypre_BoomerAMGCorrectCFMarker2 (CF_marker, local_num_vars, 
			CFN_marker);
                  hypre_TFree(CFN_marker);
                  /*hypre_TFree(coarse_dof_func);
                  coarse_dof_func = NULL;*/
                  hypre_BoomerAMGCoarseParms(comm, local_num_vars,
                        num_functions, dof_func_array[level], CF_marker,
                        &coarse_dof_func,&coarse_pnts_global);
                  /*if (num_functions > 1 && nodal > -1 && (!block_mode) )
                     dof_func_array[level+1] = coarse_dof_func;*/
	          hypre_TFree(col_offd_S_to_A);
                  if (agg_interp_type == 1)
		     hypre_BoomerAMGBuildPartialExtPIInterp(A_array[level], 
		       	CF_marker, S, coarse_pnts_global, 
			coarse_pnts_global1, num_functions, 
			dof_func_array[level], debug_flag, agg_P12_trunc_factor, 
			agg_P12_max_elmts, col_offd_S_to_A, &P2);
                  else if (agg_interp_type == 2)
		     hypre_BoomerAMGBuildPartialStdInterp(A_array[level], 
		       	CF_marker, S, coarse_pnts_global, 
			coarse_pnts_global1, num_functions, 
			dof_func_array[level], debug_flag, agg_P12_trunc_factor, 
			agg_P12_max_elmts, sep_weight, col_offd_S_to_A, &P2);
                  else if (agg_interp_type == 3)
		     hypre_BoomerAMGBuildPartialExtInterp(A_array[level], 
		       	CF_marker, S, coarse_pnts_global, 
			coarse_pnts_global1, num_functions, 
			dof_func_array[level], debug_flag, agg_P12_trunc_factor, 
			agg_P12_max_elmts, col_offd_S_to_A, &P2);
                  P = hypre_ParMatmul(P1,P2);
                  hypre_BoomerAMGInterpTruncation(P, agg_trunc_factor, 
			agg_P_max_elmts);          
	          hypre_MatvecCommPkgCreate(P);
                  hypre_ParCSRMatrixDestroy(P1);
                  hypre_ParCSRMatrixOwnsColStarts(P2) = 0;
                  hypre_ParCSRMatrixDestroy(P2);
                  hypre_ParCSRMatrixOwnsColStarts(P) = 1;
               }
            }
#ifdef HYPRE_NO_GLOBAL_PARTITION
            if (my_id == (num_procs -1)) coarse_size = coarse_pnts_global[1];
            hypre_MPI_Bcast(&coarse_size, 1, HYPRE_MPI_INT, num_procs-1, comm);
#else
            coarse_size = coarse_pnts_global[num_procs];
#endif
         }
         else /* no aggressive coarsening */
         {
            /**** Get the coarse parameters ****/
/* xxxxxxxxxxxxxxxxxxxxxxxxx change for min_coarse_size 
            if (block_mode )
            {
               hypre_BoomerAMGCoarseParms(comm,
                                          hypre_CSRMatrixNumRows(hypre_ParCSRMatrixDiag(AN)),
                                          1, NULL, CF_marker, NULL, &coarse_pnts_global);
            }
            else
            {
               hypre_BoomerAMGCoarseParms(comm, local_num_vars,
                                          num_functions, dof_func_array[level], CF_marker,
                                          &coarse_dof_func,&coarse_pnts_global);
            }
#ifdef HYPRE_NO_GLOBAL_PARTITION
            if (my_id == (num_procs -1)) coarse_size = coarse_pnts_global[1];
            hypre_MPI_Bcast(&coarse_size, 1, HYPRE_MPI_INT, num_procs-1, comm);
#else
            coarse_size = coarse_pnts_global[num_procs];
#endif
 xxxxxxxxxxxxxxxxxxxxxxxxx change for min_coarse_size */ 
         if (debug_flag==1)
         {
            wall_time = time_getWallclockSeconds() - wall_time;
            hypre_printf("Proc = %d    Level = %d    Coarsen Time = %f\n",
                       my_id,level, wall_time); 
	    fflush(NULL);
         }

            if (debug_flag==1) wall_time = time_getWallclockSeconds();

            if (interp_type == 4) 
            {
               hypre_BoomerAMGBuildMultipass(A_array[level], CF_marker, 
                  S, coarse_pnts_global, num_functions, dof_func_array[level], 
                  debug_flag, trunc_factor, P_max_elmts, sep_weight, col_offd_S_to_A, &P);
	       hypre_TFree(col_offd_S_to_A);
            }
            else if (interp_type == 2)
            {
               hypre_BoomerAMGBuildInterpHE(A_array[level], CF_marker, 
                                            S, coarse_pnts_global, num_functions, dof_func_array[level], 
                                            debug_flag, trunc_factor, P_max_elmts, col_offd_S_to_A, &P);
	       hypre_TFree(col_offd_S_to_A);
            }
            else if (interp_type == 3)
            {
               hypre_BoomerAMGBuildDirInterp(A_array[level], CF_marker, 
                                             S, coarse_pnts_global, num_functions, dof_func_array[level], 
                                             debug_flag, trunc_factor, P_max_elmts, col_offd_S_to_A, &P);
	       hypre_TFree(col_offd_S_to_A);
            }
            else if (interp_type == 6) /*Extended+i classical interpolation */
            {
               hypre_BoomerAMGBuildExtPIInterp(A_array[level], CF_marker, 
                                               S, coarse_pnts_global, num_functions, dof_func_array[level], 
                                               debug_flag, trunc_factor, P_max_elmts, col_offd_S_to_A, &P);
               hypre_TFree(col_offd_S_to_A);
            }
            else if (interp_type == 14) /*Extended classical interpolation */
            {
               hypre_BoomerAMGBuildExtInterp(A_array[level], CF_marker, 
                                             S, coarse_pnts_global, num_functions, dof_func_array[level], 
                                             debug_flag, trunc_factor, P_max_elmts, col_offd_S_to_A, &P);
               hypre_TFree(col_offd_S_to_A);
            }
            else if (interp_type == 7) /*Extended+i (if no common C) interpolation */
            {
               hypre_BoomerAMGBuildExtPICCInterp(A_array[level], CF_marker, 
                                                 S, coarse_pnts_global, num_functions, dof_func_array[level], 
                                                 debug_flag, trunc_factor, P_max_elmts, col_offd_S_to_A, &P);
               hypre_TFree(col_offd_S_to_A);
            }
            else if (interp_type == 12) /*FF interpolation */
            {
               hypre_BoomerAMGBuildFFInterp(A_array[level], CF_marker, 
                                            S, coarse_pnts_global, num_functions, dof_func_array[level], 
                                            debug_flag, trunc_factor, P_max_elmts, col_offd_S_to_A, &P);
               hypre_TFree(col_offd_S_to_A);
            }
            else if (interp_type == 13) /*FF1 interpolation */
            {
               hypre_BoomerAMGBuildFF1Interp(A_array[level], CF_marker, 
                                             S, coarse_pnts_global, num_functions, dof_func_array[level], 
                                             debug_flag, trunc_factor, P_max_elmts, col_offd_S_to_A, &P);
               hypre_TFree(col_offd_S_to_A);
           }
           else if (interp_type == 8) /*Standard interpolation */
           {
              hypre_BoomerAMGBuildStdInterp(A_array[level], CF_marker, 
                                            S, coarse_pnts_global, num_functions, dof_func_array[level], 
                                            debug_flag, trunc_factor, P_max_elmts, sep_weight, col_offd_S_to_A, &P);
	      hypre_TFree(col_offd_S_to_A);
           }
           else 
           {
              if (block_mode == 0) 
              {
                 if (nodal > -1) /* non-systems, or systems with unknown approach interpolation*/
                 {
                    /* if systems, do we want to use an interp. that uses the full strength matrix?*/
                    
                    if ( (num_functions > 1) && (interp_type == 19 || interp_type == 18 || interp_type == 17 || interp_type == 16))   
                    {
                       /* so create a second strength matrix and build interp with with num_functions = 1 */
                       hypre_BoomerAMGCreateS(A_array[level], 
                                              strong_threshold, max_row_sum, 
                                              1, dof_func_array[level],&S2);
                       col_offd_S_to_A = NULL;
                       switch (interp_type) 
                       {
                          
                          case 19:
                             dbg_flg = debug_flag;
                             if (amg_print_level) dbg_flg = -debug_flag;
                             hypre_BoomerAMGBuildInterp(A_array[level], CF_marker, 
                                      S2, coarse_pnts_global, 1, 
                                      dof_func_array[level], 
                                      dbg_flg, trunc_factor, P_max_elmts, col_offd_S_to_A, &P);
                             break;
                             
                          case 18:
                             hypre_BoomerAMGBuildStdInterp(A_array[level], CF_marker, 
                                       S2, coarse_pnts_global, 1, dof_func_array[level], 
                                       debug_flag, trunc_factor, P_max_elmts, 0, col_offd_S_to_A, &P);
                             
                             break;
                             
                          case 17:
                             hypre_BoomerAMGBuildExtPIInterp(A_array[level], CF_marker, 
                                        S2, coarse_pnts_global, 1, dof_func_array[level], 
                                        debug_flag, trunc_factor, P_max_elmts, col_offd_S_to_A, &P);
                             break;
                          case 16:
                             dbg_flg = debug_flag;
                             if (amg_print_level) dbg_flg = -debug_flag;
                             hypre_BoomerAMGBuildInterpModUnk(A_array[level], CF_marker, 
                                                              S2, coarse_pnts_global, num_functions, dof_func_array[level], 
                                                              dbg_flg, trunc_factor, P_max_elmts, col_offd_S_to_A, &P);
                             break;
                             
                       }
                  

                       hypre_ParCSRMatrixDestroy(S2);
             
                    }
                    else /* one function only or unknown-based interpolation- */
                    {
                       dbg_flg = debug_flag;
                       if (amg_print_level) dbg_flg = -debug_flag;
                       
                       hypre_BoomerAMGBuildInterp(A_array[level], CF_marker, 
                                                  S, coarse_pnts_global, num_functions, 
                                                  dof_func_array[level], 
                                                  dbg_flg, trunc_factor, P_max_elmts, col_offd_S_to_A, &P);
                       
                       
                    }
               
                    hypre_TFree(col_offd_S_to_A);
                 }
              } 
           }
            
         } /* end of no aggressive coarsening */

         /*dof_func_array[level+1] = NULL;
           if (num_functions > 1 && nodal > -1 && (!block_mode) )
            dof_func_array[level+1] = coarse_dof_func;*/


         /* store the CF array */
         CF_marker_array[level] = CF_marker;
         

         dof_func_array[level+1] = NULL;
         if (num_functions > 1 && nodal > -1 && (!block_mode) )
	    dof_func_array[level+1] = coarse_dof_func;

         
      
      } /* end of if max_levels > 1 */

      /* if no coarse-grid, stop coarsening, and set the
       * coarsest solve to be a single sweep of Jacobi */
      if ((coarse_size == 0) ||
          (coarse_size == fine_size))
      {
         HYPRE_Int     *num_grid_sweeps =
            hypre_ParAMGDataNumGridSweeps(amg_data);
         HYPRE_Int    **grid_relax_points =
            hypre_ParAMGDataGridRelaxPoints(amg_data);
         if (grid_relax_type[3] == 9 || grid_relax_type[3] == 99
          || grid_relax_type[3] == 19 || grid_relax_type[3] == 98)
	 {
	    grid_relax_type[3] = grid_relax_type[0];
	    num_grid_sweeps[3] = 1;
	    if (grid_relax_points) grid_relax_points[3][0] = 0; 
	 }
	 if (S)
            hypre_ParCSRMatrixDestroy(S);
	 if (P)
            hypre_ParCSRMatrixDestroy(P);
         if (level > 0)
         {
            /* note special case treatment of CF_marker is necessary
             * to do CF relaxation correctly when num_levels = 1 */
            hypre_TFree(CF_marker_array[level]);
            hypre_ParVectorDestroy(F_array[level]);
            hypre_ParVectorDestroy(U_array[level]);
         }

         break; 
      }
      if (level < agg_num_levels && coarse_size < min_coarse_size)
      {
	 if (S)
            hypre_ParCSRMatrixDestroy(S);
	 if (P)
            hypre_ParCSRMatrixDestroy(P);
         if (level > 0)
         {
            hypre_TFree(CF_marker_array[level]);
            hypre_ParVectorDestroy(F_array[level]);
            hypre_ParVectorDestroy(U_array[level]);
         }
         coarse_size = fine_size;

         break; 
      } 

      /*-------------------------------------------------------------
       * Build prolongation matrix, P, and place in P_array[level] 
       *--------------------------------------------------------------*/

      if (!block_mode)
      {
         if (mult_addlvl > -1 && level >= mult_addlvl && level <= add_end)
         {
            HYPRE_Real *d_diag;
            if (add_rlx == 0)
            {
               hypre_CSRMatrix *lvl_Adiag = hypre_ParCSRMatrixDiag(A_array[level]);
               HYPRE_Int lvl_nrows = hypre_CSRMatrixNumRows(lvl_Adiag);
               HYPRE_Int *lvl_i = hypre_CSRMatrixI(lvl_Adiag);
               HYPRE_Real *lvl_data = hypre_CSRMatrixData(lvl_Adiag);
               HYPRE_Real w_inv = 1.0/add_rlx_wt;
               /*HYPRE_Real w_inv = 1.0/hypre_ParAMGDataRelaxWeight(amg_data)[level];*/
               d_diag = hypre_CTAlloc(HYPRE_Real, lvl_nrows);
               for (i=0; i < lvl_nrows; i++)
		  d_diag[i] = lvl_data[lvl_i[i]]*w_inv;
            }
            else
            {
               if (num_threads == 1) 
		  hypre_ParCSRComputeL1Norms(A_array[level], 1, NULL, &d_diag);
               else 
                  hypre_ParCSRComputeL1NormsThreads(A_array[level], 1, 
			num_threads, NULL, &d_diag);
            }
            if (ns == 1)
            {
               hypre_ParCSRMatrix *Q = NULL;
               Q = hypre_ParMatmul(A_array[level],P);
               hypre_ParCSRMatrixAminvDB(P,Q,d_diag,&P_array[level]);
               A_H = hypre_ParTMatmul(P,Q);
               hypre_ParCSRMatrixRowStarts(A_H) = hypre_ParCSRMatrixColStarts(A_H);
               hypre_ParCSRMatrixOwnsRowStarts(A_H) = 1;
               hypre_ParCSRMatrixOwnsColStarts(A_H) = 0;
               hypre_ParCSRMatrixOwnsColStarts(P) = 0; 
               if (num_procs > 1) hypre_MatvecCommPkgCreate(A_H); 
               /*hypre_ParCSRMatrixDestroy(P); */
               hypre_TFree(d_diag); 
               /* Set NonGalerkin drop tol on each level */
               if (level < nongalerk_num_tol) nongalerk_tol_l = nongalerk_tol[level];
               if (nongal_tol_array) nongalerk_tol_l = nongal_tol_array[level];
               if (nongalerk_tol_l > 0.0)
               {
               /* Build Non-Galerkin Coarse Grid */
                  hypre_ParCSRMatrix *Q = NULL;
                  hypre_BoomerAMGBuildNonGalerkinCoarseOperator(&A_H, Q,
                    0.333*strong_threshold, max_row_sum, num_functions, 
                    dof_func_array[level+1], S_commpkg_switch, CF_marker_array[level], 
                    /* nongalerk_tol, sym_collapse, lump_percent, beta );*/
                      nongalerk_tol_l,      1,            0.5,    1.0 );
            
                  hypre_ParCSRMatrixColStarts(P_array[level]) = hypre_ParCSRMatrixRowStarts(A_H);
                  if (!hypre_ParCSRMatrixCommPkg(A_H))
                     hypre_MatvecCommPkgCreate(A_H);
               }
               hypre_ParCSRMatrixDestroy(Q);
			
            }
            else 
            {
               HYPRE_Int ns_tmp = ns;
               hypre_ParCSRMatrix *C = NULL;
               hypre_ParCSRMatrix *Ptmp = NULL;
               /* Set NonGalerkin drop tol on each level */
               if (level < nongalerk_num_tol)
                   nongalerk_tol_l = nongalerk_tol[level];
               if (nongal_tol_array) nongalerk_tol_l = nongal_tol_array[level];

               if (nongalerk_tol_l > 0.0)
               {
                  /* Construct AP, and then RAP */
                  hypre_ParCSRMatrix *Q = NULL;
                  Q = hypre_ParMatmul(A_array[level],P_array[level]);
                  A_H = hypre_ParTMatmul(P_array[level],Q);
                  hypre_ParCSRMatrixRowStarts(A_H) = hypre_ParCSRMatrixColStarts(A_H);
                  hypre_ParCSRMatrixOwnsRowStarts(A_H) = 1;
                  hypre_ParCSRMatrixOwnsColStarts(A_H) = 0;
                  hypre_ParCSRMatrixOwnsColStarts(P_array[level]) = 0;
                  if (num_procs > 1) hypre_MatvecCommPkgCreate(A_H);
            
                  /* Build Non-Galerkin Coarse Grid */
                  hypre_BoomerAMGBuildNonGalerkinCoarseOperator(&A_H, Q,
                    0.333*strong_threshold, max_row_sum, num_functions, 
                    dof_func_array[level+1], S_commpkg_switch, CF_marker_array[level], 
                    /* nongalerk_tol, sym_collapse, lump_percent, beta );*/
                      nongalerk_tol_l,      1,            0.5,    1.0 );
            
                  if (!hypre_ParCSRMatrixCommPkg(A_H))
                     hypre_MatvecCommPkgCreate(A_H);
            
                  /* Delete AP */
                  hypre_ParCSRMatrixDestroy(Q);
               }
               else if (rap2)
               {
                  /* Use two matrix products to generate A_H */
                  hypre_ParCSRMatrix *Q = NULL;
                  Q = hypre_ParMatmul(A_array[level],P_array[level]);
                  A_H = hypre_ParTMatmul(P_array[level],Q);
                  hypre_ParCSRMatrixOwnsRowStarts(A_H) = 1;
                  hypre_ParCSRMatrixOwnsColStarts(A_H) = 0;
                  hypre_ParCSRMatrixOwnsColStarts(P_array[level]) = 0;
                  if (num_procs > 1) hypre_MatvecCommPkgCreate(A_H);
                  /* Delete AP */
                  hypre_ParCSRMatrixDestroy(Q);
               }
               else
	          hypre_BoomerAMGBuildCoarseOperatorKT(P, A_array[level] , P, 
			keepTranspose, &A_H); 
	
               if (add_rlx == 18)
	          C = hypre_CreateC(A_array[level], 0.0);
               else
		  C = hypre_CreateC(A_array[level], add_rlx_wt);
               Ptmp = P;
	       while (ns_tmp > 0)
               {
                  Pnew = Ptmp;
                  Ptmp = NULL;
		  Ptmp = hypre_ParMatmul(C,Pnew);
                  if (ns_tmp < ns)
			hypre_ParCSRMatrixDestroy(Pnew);
		  ns_tmp--;
               }
               Pnew = Ptmp;
               P_array[level] = Pnew;
               hypre_ParCSRMatrixDestroy(C); 
            }



            if (add_P_max_elmts || add_trunc_factor)
            {
                hypre_BoomerAMGTruncandBuild(P_array[level],
                add_trunc_factor,add_P_max_elmts);
            }
            /*else
                hypre_MatvecCommPkgCreate(P_array[level]);  */
            hypre_ParCSRMatrixDestroy(P);
         }
         else
            P_array[level] = P; 
      }
     
      if (S) hypre_ParCSRMatrixDestroy(S);
      S = NULL;

      if (debug_flag==1)
      {
         wall_time = time_getWallclockSeconds() - wall_time;
         hypre_printf("Proc = %d    Level = %d    Build Interp Time = %f\n",
                     my_id,level, wall_time);
         fflush(NULL);
      }

      /*-------------------------------------------------------------
       * Build coarse-grid operator, A_array[level+1] by R*A*P
       *--------------------------------------------------------------*/

      if (debug_flag==1) wall_time = time_getWallclockSeconds();

      if (mult_addlvl == -1 || level < mult_addlvl || level > add_end)
      {
         /* Set NonGalerkin drop tol on each level */
         if (level < nongalerk_num_tol)
            nongalerk_tol_l = nongalerk_tol[level];
         if (nongal_tol_array) nongalerk_tol_l = nongal_tol_array[level];

         if (nongalerk_tol_l > 0.0)
         {
            /* Construct AP, and then RAP */
            hypre_ParCSRMatrix *Q = NULL;
            Q = hypre_ParMatmul(A_array[level],P_array[level]);
            A_H = hypre_ParTMatmul(P_array[level],Q);
            hypre_ParCSRMatrixRowStarts(A_H) = hypre_ParCSRMatrixColStarts(A_H);
            hypre_ParCSRMatrixOwnsRowStarts(A_H) = 1;
            hypre_ParCSRMatrixOwnsColStarts(A_H) = 0;
            hypre_ParCSRMatrixOwnsColStarts(P_array[level]) = 0;
            if (num_procs > 1) hypre_MatvecCommPkgCreate(A_H);
            
            /* Build Non-Galerkin Coarse Grid */
            hypre_BoomerAMGBuildNonGalerkinCoarseOperator(&A_H, Q,
                    0.333*strong_threshold, max_row_sum, num_functions, 
                    dof_func_array[level+1], S_commpkg_switch, CF_marker_array[level], 
                    /* nongalerk_tol, sym_collapse, lump_percent, beta );*/
                      nongalerk_tol_l,      1,            0.5,    1.0 );
            
            if (!hypre_ParCSRMatrixCommPkg(A_H))
                hypre_MatvecCommPkgCreate(A_H);
            
            /* Delete AP */
            hypre_ParCSRMatrixDestroy(Q);
         }
         else if (rap2)
         {
            /* Use two matrix products to generate A_H */
            hypre_ParCSRMatrix *Q = NULL;
            Q = hypre_ParMatmul(A_array[level],P_array[level]);
            A_H = hypre_ParTMatmul(P_array[level],Q);
            hypre_ParCSRMatrixOwnsRowStarts(A_H) = 1;
            hypre_ParCSRMatrixOwnsColStarts(A_H) = 0;
            hypre_ParCSRMatrixOwnsColStarts(P_array[level]) = 0;
            if (num_procs > 1) hypre_MatvecCommPkgCreate(A_H);
            /* Delete AP */
            hypre_ParCSRMatrixDestroy(Q);
         }
         else 
         {
            /* Compute standard Galerkin coarse-grid product */
            hypre_BoomerAMGBuildCoarseOperatorKT(P_array[level], A_array[level] , 
                                        P_array[level], keepTranspose, &A_H);
            if (Pnew && ns==1) 
            {
               hypre_ParCSRMatrixDestroy(P);
               P_array[level] = Pnew;
            }
	
         }

      }
 
      if (debug_flag==1)
      {
         wall_time = time_getWallclockSeconds() - wall_time;
         hypre_printf("Proc = %d    Level = %d    Build Coarse Operator Time = %f\n",
                       my_id,level, wall_time);
	 fflush(NULL);
      }

      ++level;

      if (!block_mode)
      {
         hypre_ParCSRMatrixSetNumNonzeros(A_H);
         hypre_ParCSRMatrixSetDNumNonzeros(A_H);
         A_array[level] = A_H;
      }
      
      size = ((HYPRE_Real) fine_size )*.75;
      if (coarsen_type > 0 && coarse_size >= (HYPRE_Int) size)
      {
	coarsen_type = 0;      
      }


      {
	 HYPRE_Int max_thresh = hypre_max(coarse_threshold, seq_threshold);
         if ( (level == max_levels-1) || (coarse_size <= max_thresh) )
         {
            not_finished_coarsening = 0;
         }
      }
   } 

   /* redundant coarse grid solve */
   if (  (seq_threshold >= coarse_threshold) && (coarse_size > coarse_threshold) && (level != max_levels-1))
   {
      hypre_seqAMGSetup( amg_data, level, coarse_threshold);

   }
   else if (grid_relax_type[3] == 9 || grid_relax_type[3] == 99)  /*use of Gaussian elimination on coarsest level */
   {
      if (coarse_size <= coarse_threshold)
         hypre_GaussElimSetup(amg_data, level, grid_relax_type[3]);
      else
         grid_relax_type[3] = grid_relax_type[1];
   }
   else if (grid_relax_type[3] == 19 || grid_relax_type[3] == 98)  /*use of Gaussian elimination on coarsest level */
   {
      if (coarse_size > coarse_threshold)
         grid_relax_type[3] = grid_relax_type[1];
   }

   if (level > 0)
   {
      if (block_mode == 0)
      {
         F_array[level] =
            hypre_ParVectorCreate(hypre_ParCSRMatrixComm(A_array[level]),
                                  hypre_ParCSRMatrixGlobalNumRows(A_array[level]),
                                  hypre_ParCSRMatrixRowStarts(A_array[level]));
         hypre_ParVectorInitialize(F_array[level]);
         hypre_ParVectorSetPartitioningOwner(F_array[level],0);
         
         U_array[level] =
            hypre_ParVectorCreate(hypre_ParCSRMatrixComm(A_array[level]),
                                  hypre_ParCSRMatrixGlobalNumRows(A_array[level]),
                                  hypre_ParCSRMatrixRowStarts(A_array[level]));
         hypre_ParVectorInitialize(U_array[level]);
         hypre_ParVectorSetPartitioningOwner(U_array[level],0);
      }   
   }

   /*-----------------------------------------------------------------------
    * enter all the stuff created, A[level], P[level], CF_marker[level],
    * for levels 1 through coarsest, into amg_data data structure
    *-----------------------------------------------------------------------*/

   num_levels = level+1;
   hypre_ParAMGDataNumLevels(amg_data) = num_levels;
   
   /*-----------------------------------------------------------------------
    * Setup of special smoothers when needed
    *-----------------------------------------------------------------------*/

   if (addlvl > -1 || 
	grid_relax_type[1] == 7 || grid_relax_type[2] == 7 || grid_relax_type[3] == 7 ||
	grid_relax_type[1] == 8 || grid_relax_type[2] == 8 || grid_relax_type[3] == 8 ||
	grid_relax_type[1] == 13 || grid_relax_type[2] == 13 || grid_relax_type[3] == 13 ||
	grid_relax_type[1] == 14 || grid_relax_type[2] == 14 || grid_relax_type[3] == 14 ||
	grid_relax_type[1] == 18 || grid_relax_type[2] == 18 || grid_relax_type[3] == 18)
   {
      l1_norms = hypre_CTAlloc(HYPRE_Real *, num_levels);
      hypre_ParAMGDataL1Norms(amg_data) = l1_norms;
   }
   if (grid_relax_type[0] == 16 ||grid_relax_type[1] == 16 || grid_relax_type[2] == 16 || grid_relax_type[3] == 16)
      /* Chebyshev */
   {
      max_eig_est = hypre_CTAlloc(HYPRE_Real, num_levels);
      min_eig_est = hypre_CTAlloc(HYPRE_Real, num_levels);
      hypre_ParAMGDataMaxEigEst(amg_data) = max_eig_est;
      hypre_ParAMGDataMinEigEst(amg_data) = min_eig_est;
      cheby_ds = hypre_CTAlloc(HYPRE_Real *, num_levels);
      cheby_coefs = hypre_CTAlloc(HYPRE_Real *, num_levels);
      hypre_ParAMGDataChebyDS(amg_data) = cheby_ds;
      hypre_ParAMGDataChebyCoefs(amg_data) = cheby_coefs;
   }

   if (addlvl == -1) addlvl = num_levels;
   for (j = 0; j < addlvl; j++)
   {
      if (num_threads == 1)
      {
         if (j < num_levels-1 && (grid_relax_type[1] == 8 || grid_relax_type[1] == 13 || 
		grid_relax_type[1] == 14 || grid_relax_type[2] == 8 || grid_relax_type[2] == 13 ||
		grid_relax_type[2] == 14)) 
         {
            if (relax_order)
               hypre_ParCSRComputeL1Norms(A_array[j], 4, CF_marker_array[j], &l1_norms[j]);
            else
               hypre_ParCSRComputeL1Norms(A_array[j], 4, NULL, &l1_norms[j]);
         }
         else if ((grid_relax_type[3] == 8 || grid_relax_type[3] == 13 || grid_relax_type[3] == 14) 
		&& j == num_levels-1)
         {
            hypre_ParCSRComputeL1Norms(A_array[j], 4, NULL, &l1_norms[j]);
         }
         if ((grid_relax_type[1] == 18 || grid_relax_type[2] == 18)  && j < num_levels-1)
         {
            if (relax_order)
               hypre_ParCSRComputeL1Norms(A_array[j], 1, CF_marker_array[j], &l1_norms[j]);
            else
               hypre_ParCSRComputeL1Norms(A_array[j], 1, NULL, &l1_norms[j]);
         }
         else if (grid_relax_type[3] == 18 && j == num_levels-1)
         {
            hypre_ParCSRComputeL1Norms(A_array[j], 1, NULL, &l1_norms[j]);
         }
      }
      else
      {
         if (j < num_levels-1 && (grid_relax_type[1] == 8 || grid_relax_type[1] == 13 || 
		grid_relax_type[1] == 14 || grid_relax_type[2] == 8 || grid_relax_type[2] == 13 ||
		grid_relax_type[2] == 14)) 
         {
            if (relax_order)
               hypre_ParCSRComputeL1NormsThreads(A_array[j], 4, num_threads, CF_marker_array[j] , &l1_norms[j]);
            else
               hypre_ParCSRComputeL1NormsThreads(A_array[j], 4, num_threads, NULL, &l1_norms[j]);
         }
         else if ((grid_relax_type[3] == 8 || grid_relax_type[3] == 13 || grid_relax_type[3] == 14) 
		&& j == num_levels-1)
         {
            hypre_ParCSRComputeL1NormsThreads(A_array[j], 4, num_threads, NULL, &l1_norms[j]);
         }
         if ((grid_relax_type[1] == 18 || grid_relax_type[2] == 18)  && j < num_levels-1)
         {
            if (relax_order)
               hypre_ParCSRComputeL1NormsThreads(A_array[j], 1, num_threads, CF_marker_array[j], &l1_norms[j]);
            else
               hypre_ParCSRComputeL1NormsThreads(A_array[j], 1, num_threads, NULL, &l1_norms[j]);
         }
         else if (grid_relax_type[3] == 18 && j == num_levels-1)
         {
            hypre_ParCSRComputeL1NormsThreads(A_array[j], 1, num_threads, NULL, &l1_norms[j]);
         }
      }
   }
   for (j = addlvl; j < hypre_min(add_end+1, num_levels) ; j++)
   {
      if (add_rlx == 18 )
      {
         if (num_threads == 1)
               hypre_ParCSRComputeL1Norms(A_array[j], 1, NULL, &l1_norms[j]);
         else
               hypre_ParCSRComputeL1NormsThreads(A_array[j], 1, num_threads, NULL, &l1_norms[j]);
      }
   }
   for (j = add_end+1; j < num_levels; j++)
   {
      if (num_threads == 1)
      {
         if (j < num_levels-1 && (grid_relax_type[1] == 8 || grid_relax_type[1] == 13 || 
		grid_relax_type[1] == 14 || grid_relax_type[2] == 8 || grid_relax_type[2] == 13 ||
		grid_relax_type[2] == 14)) 
         {
            if (relax_order)
               hypre_ParCSRComputeL1Norms(A_array[j], 4, CF_marker_array[j], &l1_norms[j]);
            else
               hypre_ParCSRComputeL1Norms(A_array[j], 4, NULL, &l1_norms[j]);
         }
         else if ((grid_relax_type[3] == 8 || grid_relax_type[3] == 13 || grid_relax_type[3] == 14) 
		&& j == num_levels-1)
         {
            hypre_ParCSRComputeL1Norms(A_array[j], 4, NULL, &l1_norms[j]);
         }
         if ((grid_relax_type[1] == 18 || grid_relax_type[2] == 18)  && j < num_levels-1)
         {
            if (relax_order)
               hypre_ParCSRComputeL1Norms(A_array[j], 1, CF_marker_array[j], &l1_norms[j]);
            else
               hypre_ParCSRComputeL1Norms(A_array[j], 1, NULL, &l1_norms[j]);
         }
         else if (grid_relax_type[3] == 18 && j == num_levels-1)
         {
            hypre_ParCSRComputeL1Norms(A_array[j], 1, NULL, &l1_norms[j]);
         }
      }
      else
      {
         if (j < num_levels-1 && (grid_relax_type[1] == 8 || grid_relax_type[1] == 13 || 
		grid_relax_type[1] == 14 || grid_relax_type[2] == 8 || grid_relax_type[2] == 13 ||
		grid_relax_type[2] == 14)) 
         {
            if (relax_order)
               hypre_ParCSRComputeL1NormsThreads(A_array[j], 4, num_threads, CF_marker_array[j] , &l1_norms[j]);
            else
               hypre_ParCSRComputeL1NormsThreads(A_array[j], 4, num_threads, NULL, &l1_norms[j]);
         }
         else if ((grid_relax_type[3] == 8 || grid_relax_type[3] == 13 || grid_relax_type[3] == 14) 
		&& j == num_levels-1)
         {
            hypre_ParCSRComputeL1NormsThreads(A_array[j], 4, num_threads, NULL, &l1_norms[j]);
         }
         if ((grid_relax_type[1] == 18 || grid_relax_type[2] == 18)  && j < num_levels-1)
         {
            if (relax_order)
               hypre_ParCSRComputeL1NormsThreads(A_array[j], 1, num_threads, CF_marker_array[j], &l1_norms[j]);
            else
               hypre_ParCSRComputeL1NormsThreads(A_array[j], 1, num_threads, NULL, &l1_norms[j]);
         }
         else if (grid_relax_type[3] == 18 && j == num_levels-1)
         {
            hypre_ParCSRComputeL1NormsThreads(A_array[j], 1, num_threads, NULL, &l1_norms[j]);
         }
      }
   }
   for (j = 0; j < num_levels; j++)
   {
      if (grid_relax_type[1] == 7 || grid_relax_type[2] == 7 || (grid_relax_type[3] == 7 && j== (num_levels-1)))
      {
          hypre_ParCSRComputeL1Norms(A_array[j], 5, NULL, &l1_norms[j]);
      }
      else if (grid_relax_type[1] == 16 || grid_relax_type[2] == 16 || (grid_relax_type[3] == 16 && j== (num_levels-1)))
      {
         HYPRE_Int scale = hypre_ParAMGDataChebyScale(amg_data);;
         HYPRE_Int variant = hypre_ParAMGDataChebyVariant(amg_data);
         HYPRE_Real max_eig, min_eig = 0;
         HYPRE_Real *coefs = NULL;
         HYPRE_Real *ds = NULL;
         HYPRE_Int cheby_order = hypre_ParAMGDataChebyOrder(amg_data);
         HYPRE_Int cheby_eig_est = hypre_ParAMGDataChebyEigEst(amg_data);
         HYPRE_Real cheby_fraction = hypre_ParAMGDataChebyFraction(amg_data);
         if (cheby_eig_est)
	    hypre_ParCSRMaxEigEstimateCG(A_array[j], scale, cheby_eig_est, 
		&max_eig, &min_eig);
         else
	    hypre_ParCSRMaxEigEstimate(A_array[j], scale, &max_eig);
         max_eig_est[j] = max_eig;
         min_eig_est[j] = min_eig;
         hypre_ParCSRRelax_Cheby_Setup(A_array[j],max_eig, min_eig, 
		cheby_fraction, cheby_order, scale, variant, &coefs, &ds);
         cheby_coefs[j] = coefs;
         cheby_ds[j] = ds;
     }
     if (relax_weight[j] == 0.0)
     {
        hypre_ParCSRMatrixScaledNorm(A_array[j], &relax_weight[j]);
        if (relax_weight[j] != 0.0)
           relax_weight[j] = 4.0/3.0/relax_weight[j];
        else
           hypre_error_w_msg(HYPRE_ERROR_GENERIC," Warning ! Matrix norm is zero !!!");
     }
     if ((j < num_levels-1) || ((j == num_levels-1) && (grid_relax_type[3]!= 9 && 
	grid_relax_type[3] != 99 && grid_relax_type[3] != 19 && grid_relax_type[3] != 98) 
	&& coarse_size > 9))
     {
        if (relax_weight[j] < 0 )
        {
           num_cg_sweeps = (HYPRE_Int) (-relax_weight[j]);
           hypre_BoomerAMGCGRelaxWt(amg_data, j, num_cg_sweeps,
                                 &relax_weight[j]);
        }
        if (omega[j] < 0 )
        {
           num_cg_sweeps = (HYPRE_Int) (-omega[j]);
           hypre_BoomerAMGCGRelaxWt(amg_data, j, num_cg_sweeps,
                                 &omega[j]);
        }
     }
   } /* end of levels loop */

   if ( amg_logging > 1 ) 
   {
      Residual_array= 
	hypre_ParVectorCreate(hypre_ParCSRMatrixComm(A_array[0]),
                              hypre_ParCSRMatrixGlobalNumRows(A_array[0]),
                              hypre_ParCSRMatrixRowStarts(A_array[0]) );
      hypre_ParVectorInitialize(Residual_array);
      hypre_ParVectorSetPartitioningOwner(Residual_array,0);
      hypre_ParAMGDataResidual(amg_data) = Residual_array;
   }
   else
      hypre_ParAMGDataResidual(amg_data) = NULL;

   if (simple > -1 && simple < num_levels)
         hypre_CreateDinv(amg_data);
   else if ((mult_additive > -1 && mult_additive < num_levels) || 
		(additive > -1 && additive < num_levels))
         hypre_CreateLambda(amg_data);

   cum_nnz_AP = hypre_ParCSRMatrixDNumNonzeros(A_array[0]);
   for (j = 0; j < num_levels-1; j++)
   {
      hypre_ParCSRMatrixSetDNumNonzeros(P_array[j]);
      cum_nnz_AP += hypre_ParCSRMatrixDNumNonzeros(P_array[j]);
      cum_nnz_AP += hypre_ParCSRMatrixDNumNonzeros(A_array[j+1]);
   }

   hypre_ParAMGDataCumNnzAP(amg_data) = cum_nnz_AP;
   /*-----------------------------------------------------------------------
    * Print some stuff
    *-----------------------------------------------------------------------*/

   if (amg_print_level == 1 || amg_print_level == 3)
      hypre_BoomerAMGSetupStats(amg_data,A);

/* print out matrices on all levels  */
#if DEBUG
{
   char  filename[256];

   if (block_mode)
   {
      hypre_ParCSRMatrix *temp_A;

      for (level = 0; level < num_levels; level++)
      {
         hypre_sprintf(filename, "BoomerAMG.out.A_blk.%02d.ij", level);
         temp_A =  hypre_ParCSRBlockMatrixConvertToParCSRMatrix(
            A_block_array[level]);
         hypre_ParCSRMatrixPrintIJ(temp_A, 0, 0, filename);
         hypre_ParCSRMatrixDestroy(temp_A);
      }
      
   }
   else
   {
      
      for (level = 0; level < num_levels; level++)
      {
         hypre_sprintf(filename, "BoomerAMG.out.A.%02d.ij", level);
         hypre_ParCSRMatrixPrintIJ(A_array[level], 0, 0, filename);
      }
      for (level = 0; level < (num_levels-1); level++)
      {
         hypre_sprintf(filename, "BoomerAMG.out.P.%02d.ij", level);
         hypre_ParCSRMatrixPrintIJ(P_array[level], 0, 0, filename);
      }
   }
}
#endif

   return(hypre_error_flag);
}  
