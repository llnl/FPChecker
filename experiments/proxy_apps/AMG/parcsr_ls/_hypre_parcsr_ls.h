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

#include "HYPRE_parcsr_ls.h"

#ifndef hypre_PARCSR_LS_HEADER
#define hypre_PARCSR_LS_HEADER

#include "_hypre_utilities.h"
#include "krylov.h"
#include "seq_mv.h"
#include "_hypre_parcsr_mv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { HYPRE_Int prev; HYPRE_Int next; } Link;






#ifndef hypre_ParAMG_DATA_HEADER
#define hypre_ParAMG_DATA_HEADER

#define CUMNUMIT


/*--------------------------------------------------------------------------
 * hypre_ParAMGData
 *--------------------------------------------------------------------------*/

typedef struct
{

   /* setup params */
   HYPRE_Int      max_levels;
   HYPRE_Real   strong_threshold;
   HYPRE_Real   max_row_sum;
   HYPRE_Real   trunc_factor;
   HYPRE_Real   agg_trunc_factor;
   HYPRE_Real   agg_P12_trunc_factor;
   HYPRE_Real   jacobi_trunc_threshold;
   HYPRE_Real   S_commpkg_switch;
   HYPRE_Int      measure_type;
   HYPRE_Int      setup_type;
   HYPRE_Int      coarsen_type;
   HYPRE_Int      P_max_elmts;
   HYPRE_Int      interp_type;
   HYPRE_Int      sep_weight;
   HYPRE_Int      agg_interp_type;
   HYPRE_Int      agg_P_max_elmts;
   HYPRE_Int      agg_P12_max_elmts;
   HYPRE_Int      restr_par;
   HYPRE_Int      agg_num_levels;
   HYPRE_Int      num_paths;
   HYPRE_Int      post_interp_type;
   HYPRE_Int      max_coarse_size;
   HYPRE_Int      min_coarse_size;
   HYPRE_Int      seq_threshold;
   HYPRE_Int      redundant;
   HYPRE_Int      participate;

   /* solve params */
   HYPRE_Int      max_iter;
   HYPRE_Int      min_iter;
   HYPRE_Int      cycle_type;    
   HYPRE_Int     *num_grid_sweeps;  
   HYPRE_Int     *grid_relax_type;   
   HYPRE_Int    **grid_relax_points;
   HYPRE_Int      relax_order;
   HYPRE_Int      user_coarse_relax_type;   
   HYPRE_Int      user_relax_type;   
   HYPRE_Int      user_num_sweeps;   
   HYPRE_Real     user_relax_weight;   
   HYPRE_Real     outer_wt;
   HYPRE_Real  *relax_weight; 
   HYPRE_Real  *omega;
   HYPRE_Real   tol;

   /* problem data */
   hypre_ParCSRMatrix  *A;
   HYPRE_Int      num_variables;
   HYPRE_Int      num_functions;
   HYPRE_Int      nodal;
   HYPRE_Int      nodal_levels;
   HYPRE_Int      nodal_diag;
   HYPRE_Int      num_points;
   HYPRE_Int     *dof_func;
   HYPRE_Int     *dof_point;           
   HYPRE_Int     *point_dof_map;

   /* data generated in the setup phase */
   hypre_ParCSRMatrix **A_array;
   hypre_ParVector    **F_array;
   hypre_ParVector    **U_array;
   hypre_ParCSRMatrix **P_array;
   hypre_ParCSRMatrix **R_array;
   HYPRE_Int                **CF_marker_array;
   HYPRE_Int                **dof_func_array;
   HYPRE_Int                **dof_point_array;
   HYPRE_Int                **point_dof_map_array;
   HYPRE_Int                  num_levels;
   HYPRE_Real         **l1_norms;


   HYPRE_Int block_mode;

   /* data for more complex smoothers */
   HYPRE_Real          *max_eig_est;
   HYPRE_Real          *min_eig_est;
   HYPRE_Int           cheby_eig_est;
   HYPRE_Int            cheby_order;
   HYPRE_Int           cheby_variant;
   HYPRE_Int           cheby_scale;
   HYPRE_Real           cheby_fraction;
   HYPRE_Real         **cheby_ds;
   HYPRE_Real         **cheby_coefs;

   /* data needed for non-Galerkin option */
   HYPRE_Int           nongalerk_num_tol;
   HYPRE_Real         *nongalerk_tol;
   HYPRE_Real          nongalerkin_tol;
   HYPRE_Real         *nongal_tol_array;

   /* data generated in the solve phase */
   hypre_ParVector   *Vtemp;
   hypre_Vector      *Vtemp_local;
   HYPRE_Real        *Vtemp_local_data;
   HYPRE_Real         cycle_op_count;
   hypre_ParVector   *Rtemp;
   hypre_ParVector   *Ptemp;
   hypre_ParVector   *Ztemp;

   /* log info */
   HYPRE_Int      logging;
   HYPRE_Int      num_iterations;
#ifdef CUMNUMIT
   HYPRE_Int      cum_num_iterations;
#endif
   HYPRE_Real   rel_resid_norm;
   hypre_ParVector *residual; /* available if logging>1 */

   /* output params */
   HYPRE_Int      print_level;
   char     log_file_name[256];
   HYPRE_Int      debug_flag;
   HYPRE_Real   cum_nnz_AP;

 /* enable redundant coarse grid solve */
   HYPRE_Solver   coarse_solver;
   hypre_ParCSRMatrix  *A_coarse;
   hypre_ParVector  *f_coarse;
   hypre_ParVector  *u_coarse;
   MPI_Comm   new_comm;

 /* store matrix, vector and communication info for Gaussian elimination */
   HYPRE_Real *A_mat;
   HYPRE_Real *b_vec;
   HYPRE_Int *comm_info;

 /* information for multiplication with Lambda - additive AMG */
   HYPRE_Int      additive;
   HYPRE_Int      mult_additive;
   HYPRE_Int      simple;
   HYPRE_Int      add_last_lvl;
   HYPRE_Int      add_P_max_elmts;
   HYPRE_Real     add_trunc_factor;
   HYPRE_Int      add_rlx_type;
   HYPRE_Real     add_rlx_wt;
   hypre_ParCSRMatrix *Lambda;
   hypre_ParCSRMatrix *Atilde;
   hypre_ParVector *Rtilde;
   hypre_ParVector *Xtilde;
   HYPRE_Real *D_inv;

   HYPRE_Int rap2;
   HYPRE_Int keepTranspose;

} hypre_ParAMGData;

/*--------------------------------------------------------------------------
 * Accessor functions for the hypre_AMGData structure
 *--------------------------------------------------------------------------*/

/* setup params */
		  		      
#define hypre_ParAMGDataRestriction(amg_data) ((amg_data)->restr_par)
#define hypre_ParAMGDataMaxLevels(amg_data) ((amg_data)->max_levels)
#define hypre_ParAMGDataStrongThreshold(amg_data) \
((amg_data)->strong_threshold)
#define hypre_ParAMGDataMaxRowSum(amg_data) ((amg_data)->max_row_sum)
#define hypre_ParAMGDataTruncFactor(amg_data) ((amg_data)->trunc_factor)
#define hypre_ParAMGDataAggTruncFactor(amg_data) ((amg_data)->agg_trunc_factor)
#define hypre_ParAMGDataAggP12TruncFactor(amg_data) ((amg_data)->agg_P12_trunc_factor)
#define hypre_ParAMGDataJacobiTruncThreshold(amg_data) ((amg_data)->jacobi_trunc_threshold)
#define hypre_ParAMGDataSCommPkgSwitch(amg_data) ((amg_data)->S_commpkg_switch)
#define hypre_ParAMGDataInterpType(amg_data) ((amg_data)->interp_type)
#define hypre_ParAMGDataSepWeight(amg_data) ((amg_data)->sep_weight)
#define hypre_ParAMGDataAggInterpType(amg_data) ((amg_data)->agg_interp_type)
#define hypre_ParAMGDataCoarsenType(amg_data) ((amg_data)->coarsen_type)
#define hypre_ParAMGDataMeasureType(amg_data) ((amg_data)->measure_type)
#define hypre_ParAMGDataSetupType(amg_data) ((amg_data)->setup_type)
#define hypre_ParAMGDataPMaxElmts(amg_data) ((amg_data)->P_max_elmts)
#define hypre_ParAMGDataAggPMaxElmts(amg_data) ((amg_data)->agg_P_max_elmts)
#define hypre_ParAMGDataAggP12MaxElmts(amg_data) ((amg_data)->agg_P12_max_elmts)
#define hypre_ParAMGDataNumPaths(amg_data) ((amg_data)->num_paths)
#define hypre_ParAMGDataAggNumLevels(amg_data) ((amg_data)->agg_num_levels)
#define hypre_ParAMGDataPostInterpType(amg_data) ((amg_data)->post_interp_type)
#define hypre_ParAMGDataL1Norms(amg_data) ((amg_data)->l1_norms)
 #define hypre_ParAMGDataMaxCoarseSize(amg_data) ((amg_data)->max_coarse_size)
 #define hypre_ParAMGDataMinCoarseSize(amg_data) ((amg_data)->min_coarse_size)
 #define hypre_ParAMGDataSeqThreshold(amg_data) ((amg_data)->seq_threshold)

/* solve params */

#define hypre_ParAMGDataMinIter(amg_data) ((amg_data)->min_iter)
#define hypre_ParAMGDataMaxIter(amg_data) ((amg_data)->max_iter)
#define hypre_ParAMGDataCycleType(amg_data) ((amg_data)->cycle_type)
#define hypre_ParAMGDataTol(amg_data) ((amg_data)->tol)
#define hypre_ParAMGDataNumGridSweeps(amg_data) ((amg_data)->num_grid_sweeps)
#define hypre_ParAMGDataUserCoarseRelaxType(amg_data) ((amg_data)->user_coarse_relax_type)
#define hypre_ParAMGDataUserRelaxType(amg_data) ((amg_data)->user_relax_type)
#define hypre_ParAMGDataUserRelaxWeight(amg_data) ((amg_data)->user_relax_weight)
#define hypre_ParAMGDataUserNumSweeps(amg_data) ((amg_data)->user_num_sweeps)
#define hypre_ParAMGDataGridRelaxType(amg_data) ((amg_data)->grid_relax_type)
#define hypre_ParAMGDataGridRelaxPoints(amg_data) \
((amg_data)->grid_relax_points)
#define hypre_ParAMGDataRelaxOrder(amg_data) ((amg_data)->relax_order)
#define hypre_ParAMGDataRelaxWeight(amg_data) ((amg_data)->relax_weight)
#define hypre_ParAMGDataOmega(amg_data) ((amg_data)->omega)
#define hypre_ParAMGDataOuterWt(amg_data) ((amg_data)->outer_wt)

/* problem data parameters */
#define  hypre_ParAMGDataNumVariables(amg_data)  ((amg_data)->num_variables)
#define hypre_ParAMGDataNumFunctions(amg_data) ((amg_data)->num_functions)
#define hypre_ParAMGDataNodal(amg_data) ((amg_data)->nodal)
#define hypre_ParAMGDataNodalLevels(amg_data) ((amg_data)->nodal_levels)
#define hypre_ParAMGDataNodalDiag(amg_data) ((amg_data)->nodal_diag)
#define hypre_ParAMGDataNumPoints(amg_data) ((amg_data)->num_points)
#define hypre_ParAMGDataDofFunc(amg_data) ((amg_data)->dof_func)
#define hypre_ParAMGDataDofPoint(amg_data) ((amg_data)->dof_point)
#define hypre_ParAMGDataPointDofMap(amg_data) ((amg_data)->point_dof_map)

/* data generated by the setup phase */
#define hypre_ParAMGDataCFMarkerArray(amg_data) ((amg_data)-> CF_marker_array)
#define hypre_ParAMGDataAArray(amg_data) ((amg_data)->A_array)
#define hypre_ParAMGDataFArray(amg_data) ((amg_data)->F_array)
#define hypre_ParAMGDataUArray(amg_data) ((amg_data)->U_array)
#define hypre_ParAMGDataPArray(amg_data) ((amg_data)->P_array)
#define hypre_ParAMGDataRArray(amg_data) ((amg_data)->R_array)
#define hypre_ParAMGDataDofFuncArray(amg_data) ((amg_data)->dof_func_array)
#define hypre_ParAMGDataDofPointArray(amg_data) ((amg_data)->dof_point_array)
#define hypre_ParAMGDataPointDofMapArray(amg_data) \
((amg_data)->point_dof_map_array) 
#define hypre_ParAMGDataNumLevels(amg_data) ((amg_data)->num_levels)	

#define hypre_ParAMGDataMaxEigEst(amg_data) ((amg_data)->max_eig_est)	
#define hypre_ParAMGDataMinEigEst(amg_data) ((amg_data)->min_eig_est)	
#define hypre_ParAMGDataChebyOrder(amg_data) ((amg_data)->cheby_order)
#define hypre_ParAMGDataChebyFraction(amg_data) ((amg_data)->cheby_fraction)
#define hypre_ParAMGDataChebyEigEst(amg_data) ((amg_data)->cheby_eig_est)
#define hypre_ParAMGDataChebyVariant(amg_data) ((amg_data)->cheby_variant)
#define hypre_ParAMGDataChebyScale(amg_data) ((amg_data)->cheby_scale)
#define hypre_ParAMGDataChebyDS(amg_data) ((amg_data)->cheby_ds)
#define hypre_ParAMGDataChebyCoefs(amg_data) ((amg_data)->cheby_coefs)


#define hypre_ParAMGDataBlockMode(amg_data) ((amg_data)->block_mode)


/* data generated in the solve phase */
#define hypre_ParAMGDataVtemp(amg_data) ((amg_data)->Vtemp)
#define hypre_ParAMGDataVtempLocal(amg_data) ((amg_data)->Vtemp_local)
#define hypre_ParAMGDataVtemplocalData(amg_data) ((amg_data)->Vtemp_local_data)
#define hypre_ParAMGDataCycleOpCount(amg_data) ((amg_data)->cycle_op_count)
#define hypre_ParAMGDataRtemp(amg_data) ((amg_data)->Rtemp)
#define hypre_ParAMGDataPtemp(amg_data) ((amg_data)->Ptemp)
#define hypre_ParAMGDataZtemp(amg_data) ((amg_data)->Ztemp)


/* log info data */
#define hypre_ParAMGDataLogging(amg_data) ((amg_data)->logging)
#define hypre_ParAMGDataNumIterations(amg_data) ((amg_data)->num_iterations)
#ifdef CUMNUMIT
#define hypre_ParAMGDataCumNumIterations(amg_data) ((amg_data)->cum_num_iterations)
#endif
#define hypre_ParAMGDataRelativeResidualNorm(amg_data) ((amg_data)->rel_resid_norm)
#define hypre_ParAMGDataResidual(amg_data) ((amg_data)->residual)

/* output parameters */
#define hypre_ParAMGDataPrintLevel(amg_data) ((amg_data)->print_level)
#define hypre_ParAMGDataLogFileName(amg_data) ((amg_data)->log_file_name)
#define hypre_ParAMGDataDebugFlag(amg_data)   ((amg_data)->debug_flag)
#define hypre_ParAMGDataCumNnzAP(amg_data)   ((amg_data)->cum_nnz_AP)

#define hypre_ParAMGDataCoarseSolver(amg_data) ((amg_data)->coarse_solver)
#define hypre_ParAMGDataACoarse(amg_data) ((amg_data)->A_coarse)
#define hypre_ParAMGDataFCoarse(amg_data) ((amg_data)->f_coarse)
#define hypre_ParAMGDataUCoarse(amg_data) ((amg_data)->u_coarse)
#define hypre_ParAMGDataNewComm(amg_data) ((amg_data)->new_comm)
#define hypre_ParAMGDataRedundant(amg_data) ((amg_data)->redundant)
#define hypre_ParAMGDataParticipate(amg_data) ((amg_data)->participate)

#define hypre_ParAMGDataAMat(amg_data) ((amg_data)->A_mat)
#define hypre_ParAMGDataBVec(amg_data) ((amg_data)->b_vec)
#define hypre_ParAMGDataCommInfo(amg_data) ((amg_data)->comm_info)

/* additive AMG parameters */
#define hypre_ParAMGDataAdditive(amg_data) ((amg_data)->additive)
#define hypre_ParAMGDataMultAdditive(amg_data) ((amg_data)->mult_additive)
#define hypre_ParAMGDataSimple(amg_data) ((amg_data)->simple)
#define hypre_ParAMGDataAddLastLvl(amg_data) ((amg_data)->add_last_lvl)
#define hypre_ParAMGDataMultAddPMaxElmts(amg_data) ((amg_data)->add_P_max_elmts)
#define hypre_ParAMGDataMultAddTruncFactor(amg_data) ((amg_data)->add_trunc_factor)
#define hypre_ParAMGDataAddRelaxType(amg_data) ((amg_data)->add_rlx_type)
#define hypre_ParAMGDataAddRelaxWt(amg_data) ((amg_data)->add_rlx_wt)
#define hypre_ParAMGDataLambda(amg_data) ((amg_data)->Lambda)
#define hypre_ParAMGDataAtilde(amg_data) ((amg_data)->Atilde)
#define hypre_ParAMGDataRtilde(amg_data) ((amg_data)->Rtilde)
#define hypre_ParAMGDataXtilde(amg_data) ((amg_data)->Xtilde)
#define hypre_ParAMGDataDinv(amg_data) ((amg_data)->D_inv)

/* non-Galerkin parameters */
#define hypre_ParAMGDataNonGalerkNumTol(amg_data) ((amg_data)->nongalerk_num_tol)
#define hypre_ParAMGDataNonGalerkTol(amg_data) ((amg_data)->nongalerk_tol)
#define hypre_ParAMGDataNonGalerkinTol(amg_data) ((amg_data)->nongalerkin_tol)
#define hypre_ParAMGDataNonGalTolArray(amg_data) ((amg_data)->nongal_tol_array)

#define hypre_ParAMGDataRAP2(amg_data) ((amg_data)->rap2)
#define hypre_ParAMGDataKeepTranspose(amg_data) ((amg_data)->keepTranspose)
#endif

/* ams.c */
HYPRE_Int hypre_ParCSRRelax ( hypre_ParCSRMatrix *A , hypre_ParVector *f , HYPRE_Int relax_type , HYPRE_Int relax_times , HYPRE_Real *l1_norms , HYPRE_Real relax_weight , HYPRE_Real omega , HYPRE_Real max_eig_est , HYPRE_Real min_eig_est , HYPRE_Int cheby_order , HYPRE_Real cheby_fraction , hypre_ParVector *u , hypre_ParVector *v , hypre_ParVector *z );
hypre_ParVector *hypre_ParVectorInRangeOf ( hypre_ParCSRMatrix *A );
hypre_ParVector *hypre_ParVectorInDomainOf ( hypre_ParCSRMatrix *A );
HYPRE_Int hypre_ParVectorBlockSplit ( hypre_ParVector *x , hypre_ParVector *x_ [3 ], HYPRE_Int dim );
HYPRE_Int hypre_ParVectorBlockGather ( hypre_ParVector *x , hypre_ParVector *x_ [3 ], HYPRE_Int dim );
HYPRE_Int hypre_BoomerAMGBlockSolve ( void *B , hypre_ParCSRMatrix *A , hypre_ParVector *b , hypre_ParVector *x );
HYPRE_Int hypre_ParCSRMatrixFixZeroRows ( hypre_ParCSRMatrix *A );
HYPRE_Int hypre_ParCSRComputeL1Norms ( hypre_ParCSRMatrix *A , HYPRE_Int option , HYPRE_Int *cf_marker , HYPRE_Real **l1_norm_ptr );
HYPRE_Int hypre_ParCSRMatrixSetDiagRows ( hypre_ParCSRMatrix *A , HYPRE_Real d );
void *hypre_AMSCreate ( void );
HYPRE_Int hypre_AMSDestroy ( void *solver );
HYPRE_Int hypre_AMSSetDimension ( void *solver , HYPRE_Int dim );
HYPRE_Int hypre_AMSSetDiscreteGradient ( void *solver , hypre_ParCSRMatrix *G );
HYPRE_Int hypre_AMSSetCoordinateVectors ( void *solver , hypre_ParVector *x , hypre_ParVector *y , hypre_ParVector *z );
HYPRE_Int hypre_AMSSetEdgeConstantVectors ( void *solver , hypre_ParVector *Gx , hypre_ParVector *Gy , hypre_ParVector *Gz );
HYPRE_Int hypre_AMSSetInterpolations ( void *solver , hypre_ParCSRMatrix *Pi , hypre_ParCSRMatrix *Pix , hypre_ParCSRMatrix *Piy , hypre_ParCSRMatrix *Piz );
HYPRE_Int hypre_AMSSetAlphaPoissonMatrix ( void *solver , hypre_ParCSRMatrix *A_Pi );
HYPRE_Int hypre_AMSSetBetaPoissonMatrix ( void *solver , hypre_ParCSRMatrix *A_G );
HYPRE_Int hypre_AMSSetInteriorNodes ( void *solver , hypre_ParVector *interior_nodes );
HYPRE_Int hypre_AMSSetProjectionFrequency ( void *solver , HYPRE_Int projection_frequency );
HYPRE_Int hypre_AMSSetMaxIter ( void *solver , HYPRE_Int maxit );
HYPRE_Int hypre_AMSSetTol ( void *solver , HYPRE_Real tol );
HYPRE_Int hypre_AMSSetCycleType ( void *solver , HYPRE_Int cycle_type );
HYPRE_Int hypre_AMSSetPrintLevel ( void *solver , HYPRE_Int print_level );
HYPRE_Int hypre_AMSSetSmoothingOptions ( void *solver , HYPRE_Int A_relax_type , HYPRE_Int A_relax_times , HYPRE_Real A_relax_weight , HYPRE_Real A_omega );
HYPRE_Int hypre_AMSSetChebySmoothingOptions ( void *solver , HYPRE_Int A_cheby_order , HYPRE_Int A_cheby_fraction );
HYPRE_Int hypre_AMSSetAlphaAMGOptions ( void *solver , HYPRE_Int B_Pi_coarsen_type , HYPRE_Int B_Pi_agg_levels , HYPRE_Int B_Pi_relax_type , HYPRE_Real B_Pi_theta , HYPRE_Int B_Pi_interp_type , HYPRE_Int B_Pi_Pmax );
HYPRE_Int hypre_AMSSetAlphaAMGCoarseRelaxType ( void *solver , HYPRE_Int B_Pi_coarse_relax_type );
HYPRE_Int hypre_AMSSetBetaAMGOptions ( void *solver , HYPRE_Int B_G_coarsen_type , HYPRE_Int B_G_agg_levels , HYPRE_Int B_G_relax_type , HYPRE_Real B_G_theta , HYPRE_Int B_G_interp_type , HYPRE_Int B_G_Pmax );
HYPRE_Int hypre_AMSSetBetaAMGCoarseRelaxType ( void *solver , HYPRE_Int B_G_coarse_relax_type );
HYPRE_Int hypre_AMSComputePi ( hypre_ParCSRMatrix *A , hypre_ParCSRMatrix *G , hypre_ParVector *Gx , hypre_ParVector *Gy , hypre_ParVector *Gz , HYPRE_Int dim , hypre_ParCSRMatrix **Pi_ptr );
HYPRE_Int hypre_AMSComputePixyz ( hypre_ParCSRMatrix *A , hypre_ParCSRMatrix *G , hypre_ParVector *Gx , hypre_ParVector *Gy , hypre_ParVector *Gz , HYPRE_Int dim , hypre_ParCSRMatrix **Pix_ptr , hypre_ParCSRMatrix **Piy_ptr , hypre_ParCSRMatrix **Piz_ptr );
HYPRE_Int hypre_AMSComputeGPi ( hypre_ParCSRMatrix *A , hypre_ParCSRMatrix *G , hypre_ParVector *Gx , hypre_ParVector *Gy , hypre_ParVector *Gz , HYPRE_Int dim , hypre_ParCSRMatrix **GPi_ptr );
HYPRE_Int hypre_AMSSetup ( void *solver , hypre_ParCSRMatrix *A , hypre_ParVector *b , hypre_ParVector *x );
HYPRE_Int hypre_AMSSolve ( void *solver , hypre_ParCSRMatrix *A , hypre_ParVector *b , hypre_ParVector *x );
HYPRE_Int hypre_ParCSRSubspacePrec ( hypre_ParCSRMatrix *A0 , HYPRE_Int A0_relax_type , HYPRE_Int A0_relax_times , HYPRE_Real *A0_l1_norms , HYPRE_Real A0_relax_weight , HYPRE_Real A0_omega , HYPRE_Real A0_max_eig_est , HYPRE_Real A0_min_eig_est , HYPRE_Int A0_cheby_order , HYPRE_Real A0_cheby_fraction , hypre_ParCSRMatrix **A , HYPRE_Solver *B , HYPRE_PtrToSolverFcn *HB , hypre_ParCSRMatrix **P , hypre_ParVector **r , hypre_ParVector **g , hypre_ParVector *x , hypre_ParVector *y , hypre_ParVector *r0 , hypre_ParVector *g0 , char *cycle , hypre_ParVector *z );
HYPRE_Int hypre_AMSGetNumIterations ( void *solver , HYPRE_Int *num_iterations );
HYPRE_Int hypre_AMSGetFinalRelativeResidualNorm ( void *solver , HYPRE_Real *rel_resid_norm );
HYPRE_Int hypre_AMSProjectOutGradients ( void *solver , hypre_ParVector *x );
HYPRE_Int hypre_AMSConstructDiscreteGradient ( hypre_ParCSRMatrix *A , hypre_ParVector *x_coord , HYPRE_Int *edge_vertex , HYPRE_Int edge_orientation , hypre_ParCSRMatrix **G_ptr );
HYPRE_Int hypre_AMSFEISetup ( void *solver , hypre_ParCSRMatrix *A , hypre_ParVector *b , hypre_ParVector *x , HYPRE_Int num_vert , HYPRE_Int num_local_vert , HYPRE_Int *vert_number , HYPRE_Real *vert_coord , HYPRE_Int num_edges , HYPRE_Int *edge_vertex );
HYPRE_Int hypre_AMSFEIDestroy ( void *solver );
HYPRE_Int hypre_ParCSRComputeL1NormsThreads ( hypre_ParCSRMatrix *A , HYPRE_Int option , HYPRE_Int num_threads , HYPRE_Int *cf_marker , HYPRE_Real **l1_norm_ptr );
HYPRE_Int hypre_ParCSRRelaxThreads ( hypre_ParCSRMatrix *A , hypre_ParVector *f , HYPRE_Int relax_type , HYPRE_Int relax_times , HYPRE_Real *l1_norms , HYPRE_Real relax_weight , HYPRE_Real omega , hypre_ParVector *u , hypre_ParVector *Vtemp , hypre_ParVector *z );


/* aux_interp.c */
HYPRE_Int hypre_alt_insert_new_nodes ( hypre_ParCSRCommPkg *comm_pkg , hypre_ParCSRCommPkg *extend_comm_pkg , HYPRE_Int *IN_marker , HYPRE_Int full_off_procNodes , HYPRE_Int *OUT_marker );
HYPRE_Int hypre_ParCSRFindExtendCommPkg ( hypre_ParCSRMatrix *A , HYPRE_Int newoff , HYPRE_Int *found , hypre_ParCSRCommPkg **extend_comm_pkg );
HYPRE_Int hypre_ssort ( HYPRE_Int *data , HYPRE_Int n );
HYPRE_Int hypre_index_of_minimum ( HYPRE_Int *data , HYPRE_Int n );
void hypre_swap_int ( HYPRE_Int *data , HYPRE_Int a , HYPRE_Int b );
void hypre_initialize_vecs ( HYPRE_Int diag_n , HYPRE_Int offd_n , HYPRE_Int *diag_ftc , HYPRE_Int *offd_ftc , HYPRE_Int *diag_pm , HYPRE_Int *offd_pm , HYPRE_Int *tmp_CF );
/*HYPRE_Int hypre_new_offd_nodes(HYPRE_Int **found , HYPRE_Int num_cols_A_offd , HYPRE_Int *A_ext_i , HYPRE_Int *A_ext_j, HYPRE_Int num_cols_S_offd, HYPRE_Int *col_map_offd, HYPRE_Int col_1, HYPRE_Int col_n, HYPRE_Int *Sop_i, HYPRE_Int *Sop_j, HYPRE_Int *CF_marker_offd );*/
HYPRE_Int hypre_exchange_marker(hypre_ParCSRCommPkg *comm_pkg, HYPRE_Int *IN_marker, HYPRE_Int *OUT_marker);
HYPRE_Int hypre_exchange_interp_data( HYPRE_Int **CF_marker_offd, HYPRE_Int **dof_func_offd, hypre_CSRMatrix **A_ext, HYPRE_Int *full_off_procNodes, hypre_CSRMatrix **Sop, hypre_ParCSRCommPkg **extend_comm_pkg, hypre_ParCSRMatrix *A, HYPRE_Int *CF_marker, hypre_ParCSRMatrix *S, HYPRE_Int num_functions, HYPRE_Int *dof_func, HYPRE_Int skip_fine_or_same_sign);
void hypre_build_interp_colmap(hypre_ParCSRMatrix *P, HYPRE_Int full_off_procNodes, HYPRE_Int *tmp_CF_marker_offd, HYPRE_Int *fine_to_coarse_offd);

/* gen_redcs_mat.c */
HYPRE_Int hypre_seqAMGSetup ( hypre_ParAMGData *amg_data , HYPRE_Int p_level , HYPRE_Int coarse_threshold );
HYPRE_Int hypre_seqAMGCycle ( hypre_ParAMGData *amg_data , HYPRE_Int p_level , hypre_ParVector **Par_F_array , hypre_ParVector **Par_U_array );
HYPRE_Int hypre_GenerateSubComm ( MPI_Comm comm , HYPRE_Int participate , MPI_Comm *new_comm_ptr );
void hypre_merge_lists ( HYPRE_Int *list1 , HYPRE_Int *list2 , hypre_int *np1 , hypre_MPI_Datatype *dptr );

/* HYPRE_parcsr_amg.c */
HYPRE_Int HYPRE_BoomerAMGCreate ( HYPRE_Solver *solver );
HYPRE_Int HYPRE_BoomerAMGDestroy ( HYPRE_Solver solver );
HYPRE_Int HYPRE_BoomerAMGSetup ( HYPRE_Solver solver , HYPRE_ParCSRMatrix A , HYPRE_ParVector b , HYPRE_ParVector x );
HYPRE_Int HYPRE_BoomerAMGSolve ( HYPRE_Solver solver , HYPRE_ParCSRMatrix A , HYPRE_ParVector b , HYPRE_ParVector x );
HYPRE_Int HYPRE_BoomerAMGSolveT ( HYPRE_Solver solver , HYPRE_ParCSRMatrix A , HYPRE_ParVector b , HYPRE_ParVector x );
HYPRE_Int HYPRE_BoomerAMGSetRestriction ( HYPRE_Solver solver , HYPRE_Int restr_par );
HYPRE_Int HYPRE_BoomerAMGSetMaxLevels ( HYPRE_Solver solver , HYPRE_Int max_levels );
HYPRE_Int HYPRE_BoomerAMGGetMaxLevels ( HYPRE_Solver solver , HYPRE_Int *max_levels );
HYPRE_Int HYPRE_BoomerAMGSetMaxCoarseSize ( HYPRE_Solver solver , HYPRE_Int max_coarse_size );
HYPRE_Int HYPRE_BoomerAMGGetMaxCoarseSize ( HYPRE_Solver solver , HYPRE_Int *max_coarse_size );
HYPRE_Int HYPRE_BoomerAMGSetMinCoarseSize ( HYPRE_Solver solver , HYPRE_Int min_coarse_size );
HYPRE_Int HYPRE_BoomerAMGGetMinCoarseSize ( HYPRE_Solver solver , HYPRE_Int *min_coarse_size );
HYPRE_Int HYPRE_BoomerAMGSetSeqThreshold ( HYPRE_Solver solver , HYPRE_Int seq_threshold );
HYPRE_Int HYPRE_BoomerAMGGetSeqThreshold ( HYPRE_Solver solver , HYPRE_Int *seq_threshold );
HYPRE_Int HYPRE_BoomerAMGSetRedundant ( HYPRE_Solver solver , HYPRE_Int redundant );
HYPRE_Int HYPRE_BoomerAMGGetRedundant ( HYPRE_Solver solver , HYPRE_Int *redundant );
HYPRE_Int HYPRE_BoomerAMGSetStrongThreshold ( HYPRE_Solver solver , HYPRE_Real strong_threshold );
HYPRE_Int HYPRE_BoomerAMGGetStrongThreshold ( HYPRE_Solver solver , HYPRE_Real *strong_threshold );
HYPRE_Int HYPRE_BoomerAMGSetMaxRowSum ( HYPRE_Solver solver , HYPRE_Real max_row_sum );
HYPRE_Int HYPRE_BoomerAMGGetMaxRowSum ( HYPRE_Solver solver , HYPRE_Real *max_row_sum );
HYPRE_Int HYPRE_BoomerAMGSetTruncFactor ( HYPRE_Solver solver , HYPRE_Real trunc_factor );
HYPRE_Int HYPRE_BoomerAMGGetTruncFactor ( HYPRE_Solver solver , HYPRE_Real *trunc_factor );
HYPRE_Int HYPRE_BoomerAMGSetPMaxElmts ( HYPRE_Solver solver , HYPRE_Int P_max_elmts );
HYPRE_Int HYPRE_BoomerAMGGetPMaxElmts ( HYPRE_Solver solver , HYPRE_Int *P_max_elmts );
HYPRE_Int HYPRE_BoomerAMGSetJacobiTruncThreshold ( HYPRE_Solver solver , HYPRE_Real jacobi_trunc_threshold );
HYPRE_Int HYPRE_BoomerAMGGetJacobiTruncThreshold ( HYPRE_Solver solver , HYPRE_Real *jacobi_trunc_threshold );
HYPRE_Int HYPRE_BoomerAMGSetPostInterpType ( HYPRE_Solver solver , HYPRE_Int post_interp_type );
HYPRE_Int HYPRE_BoomerAMGGetPostInterpType ( HYPRE_Solver solver , HYPRE_Int *post_interp_type );
HYPRE_Int HYPRE_BoomerAMGSetSCommPkgSwitch ( HYPRE_Solver solver , HYPRE_Real S_commpkg_switch );
HYPRE_Int HYPRE_BoomerAMGSetInterpType ( HYPRE_Solver solver , HYPRE_Int interp_type );
HYPRE_Int HYPRE_BoomerAMGSetSepWeight ( HYPRE_Solver solver , HYPRE_Int sep_weight );
HYPRE_Int HYPRE_BoomerAMGSetMinIter ( HYPRE_Solver solver , HYPRE_Int min_iter );
HYPRE_Int HYPRE_BoomerAMGSetMaxIter ( HYPRE_Solver solver , HYPRE_Int max_iter );
HYPRE_Int HYPRE_BoomerAMGGetMaxIter ( HYPRE_Solver solver , HYPRE_Int *max_iter );
HYPRE_Int HYPRE_BoomerAMGSetCoarsenType ( HYPRE_Solver solver , HYPRE_Int coarsen_type );
HYPRE_Int HYPRE_BoomerAMGGetCoarsenType ( HYPRE_Solver solver , HYPRE_Int *coarsen_type );
HYPRE_Int HYPRE_BoomerAMGSetMeasureType ( HYPRE_Solver solver , HYPRE_Int measure_type );
HYPRE_Int HYPRE_BoomerAMGGetMeasureType ( HYPRE_Solver solver , HYPRE_Int *measure_type );
HYPRE_Int HYPRE_BoomerAMGSetSetupType ( HYPRE_Solver solver , HYPRE_Int setup_type );
HYPRE_Int HYPRE_BoomerAMGSetOldDefault ( HYPRE_Solver solver );
HYPRE_Int HYPRE_BoomerAMGSetCycleType ( HYPRE_Solver solver , HYPRE_Int cycle_type );
HYPRE_Int HYPRE_BoomerAMGGetCycleType ( HYPRE_Solver solver , HYPRE_Int *cycle_type );
HYPRE_Int HYPRE_BoomerAMGSetTol ( HYPRE_Solver solver , HYPRE_Real tol );
HYPRE_Int HYPRE_BoomerAMGGetTol ( HYPRE_Solver solver , HYPRE_Real *tol );
HYPRE_Int HYPRE_BoomerAMGSetNumGridSweeps ( HYPRE_Solver solver , HYPRE_Int *num_grid_sweeps );
HYPRE_Int HYPRE_BoomerAMGSetNumSweeps ( HYPRE_Solver solver , HYPRE_Int num_sweeps );
HYPRE_Int HYPRE_BoomerAMGSetCycleNumSweeps ( HYPRE_Solver solver , HYPRE_Int num_sweeps , HYPRE_Int k );
HYPRE_Int HYPRE_BoomerAMGGetCycleNumSweeps ( HYPRE_Solver solver , HYPRE_Int *num_sweeps , HYPRE_Int k );
HYPRE_Int HYPRE_BoomerAMGInitGridRelaxation ( HYPRE_Int **num_grid_sweeps_ptr , HYPRE_Int **grid_relax_type_ptr , HYPRE_Int ***grid_relax_points_ptr , HYPRE_Int coarsen_type , HYPRE_Real **relax_weights_ptr , HYPRE_Int max_levels );
HYPRE_Int HYPRE_BoomerAMGSetGridRelaxType ( HYPRE_Solver solver , HYPRE_Int *grid_relax_type );
HYPRE_Int HYPRE_BoomerAMGSetRelaxType ( HYPRE_Solver solver , HYPRE_Int relax_type );
HYPRE_Int HYPRE_BoomerAMGSetCycleRelaxType ( HYPRE_Solver solver , HYPRE_Int relax_type , HYPRE_Int k );
HYPRE_Int HYPRE_BoomerAMGGetCycleRelaxType ( HYPRE_Solver solver , HYPRE_Int *relax_type , HYPRE_Int k );
HYPRE_Int HYPRE_BoomerAMGSetRelaxOrder ( HYPRE_Solver solver , HYPRE_Int relax_order );
HYPRE_Int HYPRE_BoomerAMGSetGridRelaxPoints ( HYPRE_Solver solver , HYPRE_Int **grid_relax_points );
HYPRE_Int HYPRE_BoomerAMGSetRelaxWeight ( HYPRE_Solver solver , HYPRE_Real *relax_weight );
HYPRE_Int HYPRE_BoomerAMGSetRelaxWt ( HYPRE_Solver solver , HYPRE_Real relax_wt );
HYPRE_Int HYPRE_BoomerAMGSetLevelRelaxWt ( HYPRE_Solver solver , HYPRE_Real relax_wt , HYPRE_Int level );
HYPRE_Int HYPRE_BoomerAMGSetOmega ( HYPRE_Solver solver , HYPRE_Real *omega );
HYPRE_Int HYPRE_BoomerAMGSetOuterWt ( HYPRE_Solver solver , HYPRE_Real outer_wt );
HYPRE_Int HYPRE_BoomerAMGSetLevelOuterWt ( HYPRE_Solver solver , HYPRE_Real outer_wt , HYPRE_Int level );
HYPRE_Int HYPRE_BoomerAMGSetLogging ( HYPRE_Solver solver , HYPRE_Int logging );
HYPRE_Int HYPRE_BoomerAMGGetLogging ( HYPRE_Solver solver , HYPRE_Int *logging );
HYPRE_Int HYPRE_BoomerAMGSetPrintLevel ( HYPRE_Solver solver , HYPRE_Int print_level );
HYPRE_Int HYPRE_BoomerAMGGetPrintLevel ( HYPRE_Solver solver , HYPRE_Int *print_level );
HYPRE_Int HYPRE_BoomerAMGSetPrintFileName ( HYPRE_Solver solver , const char *print_file_name );
HYPRE_Int HYPRE_BoomerAMGSetDebugFlag ( HYPRE_Solver solver , HYPRE_Int debug_flag );
HYPRE_Int HYPRE_BoomerAMGGetDebugFlag ( HYPRE_Solver solver , HYPRE_Int *debug_flag );
HYPRE_Int HYPRE_BoomerAMGGetNumIterations ( HYPRE_Solver solver , HYPRE_Int *num_iterations );
HYPRE_Int HYPRE_BoomerAMGGetCumNumIterations ( HYPRE_Solver solver , HYPRE_Int *cum_num_iterations );
HYPRE_Int HYPRE_BoomerAMGGetCumNnzAP ( HYPRE_Solver solver , HYPRE_Real *cum_nnz_AP );
HYPRE_Int HYPRE_BoomerAMGGetResidual ( HYPRE_Solver solver , HYPRE_ParVector *residual );
HYPRE_Int HYPRE_BoomerAMGGetFinalRelativeResidualNorm ( HYPRE_Solver solver , HYPRE_Real *rel_resid_norm );
HYPRE_Int HYPRE_BoomerAMGSetNumFunctions ( HYPRE_Solver solver , HYPRE_Int num_functions );
HYPRE_Int HYPRE_BoomerAMGGetNumFunctions ( HYPRE_Solver solver , HYPRE_Int *num_functions );
HYPRE_Int HYPRE_BoomerAMGSetNodal ( HYPRE_Solver solver , HYPRE_Int nodal );
HYPRE_Int HYPRE_BoomerAMGSetNodalLevels ( HYPRE_Solver solver , HYPRE_Int nodal_levels );
HYPRE_Int HYPRE_BoomerAMGSetNodalDiag ( HYPRE_Solver solver , HYPRE_Int nodal );
HYPRE_Int HYPRE_BoomerAMGSetDofFunc ( HYPRE_Solver solver , HYPRE_Int *dof_func );
HYPRE_Int HYPRE_BoomerAMGSetNumPaths ( HYPRE_Solver solver , HYPRE_Int num_paths );
HYPRE_Int HYPRE_BoomerAMGSetAggNumLevels ( HYPRE_Solver solver , HYPRE_Int agg_num_levels );
HYPRE_Int HYPRE_BoomerAMGSetAggInterpType ( HYPRE_Solver solver , HYPRE_Int agg_interp_type );
HYPRE_Int HYPRE_BoomerAMGSetAggTruncFactor ( HYPRE_Solver solver , HYPRE_Real agg_trunc_factor );
HYPRE_Int HYPRE_BoomerAMGSetAddTruncFactor ( HYPRE_Solver solver , HYPRE_Real add_trunc_factor );
HYPRE_Int HYPRE_BoomerAMGSetMultAddTruncFactor ( HYPRE_Solver solver , HYPRE_Real add_trunc_factor );
HYPRE_Int HYPRE_BoomerAMGSetAggP12TruncFactor ( HYPRE_Solver solver , HYPRE_Real agg_P12_trunc_factor );
HYPRE_Int HYPRE_BoomerAMGSetAggPMaxElmts ( HYPRE_Solver solver , HYPRE_Int agg_P_max_elmts );
HYPRE_Int HYPRE_BoomerAMGSetAddPMaxElmts ( HYPRE_Solver solver , HYPRE_Int add_P_max_elmts );
HYPRE_Int HYPRE_BoomerAMGSetMultAddPMaxElmts ( HYPRE_Solver solver , HYPRE_Int add_P_max_elmts );
HYPRE_Int HYPRE_BoomerAMGSetAddRelaxType ( HYPRE_Solver solver , HYPRE_Int add_rlx_type );
HYPRE_Int HYPRE_BoomerAMGSetAddRelaxWt ( HYPRE_Solver solver , HYPRE_Real add_rlx_wt );
HYPRE_Int HYPRE_BoomerAMGSetAggP12MaxElmts ( HYPRE_Solver solver , HYPRE_Int agg_P12_max_elmts );
HYPRE_Int HYPRE_BoomerAMGSetChebyOrder ( HYPRE_Solver solver , HYPRE_Int order );
HYPRE_Int HYPRE_BoomerAMGSetChebyFraction ( HYPRE_Solver solver , HYPRE_Real ratio );
HYPRE_Int HYPRE_BoomerAMGSetChebyEigEst ( HYPRE_Solver solver , HYPRE_Int eig_est );
HYPRE_Int HYPRE_BoomerAMGSetChebyVariant ( HYPRE_Solver solver , HYPRE_Int variant );
HYPRE_Int HYPRE_BoomerAMGSetChebyScale ( HYPRE_Solver solver , HYPRE_Int scale );
HYPRE_Int HYPRE_BoomerAMGSetAdditive ( HYPRE_Solver solver , HYPRE_Int additive );
HYPRE_Int HYPRE_BoomerAMGGetAdditive ( HYPRE_Solver solver , HYPRE_Int *additive );
HYPRE_Int HYPRE_BoomerAMGSetMultAdditive ( HYPRE_Solver solver , HYPRE_Int mult_additive );
HYPRE_Int HYPRE_BoomerAMGGetMultAdditive ( HYPRE_Solver solver , HYPRE_Int *mult_additive );
HYPRE_Int HYPRE_BoomerAMGSetSimple ( HYPRE_Solver solver , HYPRE_Int simple );
HYPRE_Int HYPRE_BoomerAMGGetSimple ( HYPRE_Solver solver , HYPRE_Int *simple );
HYPRE_Int HYPRE_BoomerAMGSetAddLastLvl ( HYPRE_Solver solver , HYPRE_Int add_last_lvl );
HYPRE_Int HYPRE_BoomerAMGSetNonGalerkinTol ( HYPRE_Solver solver , HYPRE_Real nongalerkin_tol );
HYPRE_Int HYPRE_BoomerAMGSetLevelNonGalerkinTol ( HYPRE_Solver solver , HYPRE_Real nongalerkin_tol , HYPRE_Int level );
HYPRE_Int HYPRE_BoomerAMGSetNonGalerkTol ( HYPRE_Solver solver , HYPRE_Int nongalerk_num_tol , HYPRE_Real *nongalerk_tol );
HYPRE_Int HYPRE_BoomerAMGSetRAP2 ( HYPRE_Solver solver , HYPRE_Int rap2 );
HYPRE_Int HYPRE_BoomerAMGSetKeepTranspose ( HYPRE_Solver solver , HYPRE_Int keepTranspose );

/* HYPRE_parcsr_gmres.c */
HYPRE_Int HYPRE_ParCSRGMRESCreate ( MPI_Comm comm , HYPRE_Solver *solver );
HYPRE_Int HYPRE_ParCSRGMRESDestroy ( HYPRE_Solver solver );
HYPRE_Int HYPRE_ParCSRGMRESSetup ( HYPRE_Solver solver , HYPRE_ParCSRMatrix A , HYPRE_ParVector b , HYPRE_ParVector x );
HYPRE_Int HYPRE_ParCSRGMRESSolve ( HYPRE_Solver solver , HYPRE_ParCSRMatrix A , HYPRE_ParVector b , HYPRE_ParVector x );
HYPRE_Int HYPRE_ParCSRGMRESSetKDim ( HYPRE_Solver solver , HYPRE_Int k_dim );
HYPRE_Int HYPRE_ParCSRGMRESSetTol ( HYPRE_Solver solver , HYPRE_Real tol );
HYPRE_Int HYPRE_ParCSRGMRESSetAbsoluteTol ( HYPRE_Solver solver , HYPRE_Real a_tol );
HYPRE_Int HYPRE_ParCSRGMRESSetMinIter ( HYPRE_Solver solver , HYPRE_Int min_iter );
HYPRE_Int HYPRE_ParCSRGMRESSetMaxIter ( HYPRE_Solver solver , HYPRE_Int max_iter );
HYPRE_Int HYPRE_ParCSRGMRESSetStopCrit ( HYPRE_Solver solver , HYPRE_Int stop_crit );
HYPRE_Int HYPRE_ParCSRGMRESSetPrecond ( HYPRE_Solver solver , HYPRE_PtrToParSolverFcn precond , HYPRE_PtrToParSolverFcn precond_setup , HYPRE_Solver precond_solver );
HYPRE_Int HYPRE_ParCSRGMRESGetPrecond ( HYPRE_Solver solver , HYPRE_Solver *precond_data_ptr );
HYPRE_Int HYPRE_ParCSRGMRESSetLogging ( HYPRE_Solver solver , HYPRE_Int logging );
HYPRE_Int HYPRE_ParCSRGMRESSetPrintLevel ( HYPRE_Solver solver , HYPRE_Int print_level );
HYPRE_Int HYPRE_ParCSRGMRESGetNumIterations ( HYPRE_Solver solver , HYPRE_Int *num_iterations );
HYPRE_Int HYPRE_ParCSRGMRESGetFinalRelativeResidualNorm ( HYPRE_Solver solver , HYPRE_Real *norm );

/* HYPRE_parcsr_pcg.c */
HYPRE_Int HYPRE_ParCSRPCGCreate ( MPI_Comm comm , HYPRE_Solver *solver );
HYPRE_Int HYPRE_ParCSRPCGDestroy ( HYPRE_Solver solver );
HYPRE_Int HYPRE_ParCSRPCGSetup ( HYPRE_Solver solver , HYPRE_ParCSRMatrix A , HYPRE_ParVector b , HYPRE_ParVector x );
HYPRE_Int HYPRE_ParCSRPCGSolve ( HYPRE_Solver solver , HYPRE_ParCSRMatrix A , HYPRE_ParVector b , HYPRE_ParVector x );
HYPRE_Int HYPRE_ParCSRPCGSetTol ( HYPRE_Solver solver , HYPRE_Real tol );
HYPRE_Int HYPRE_ParCSRPCGSetAbsoluteTol ( HYPRE_Solver solver , HYPRE_Real a_tol );
HYPRE_Int HYPRE_ParCSRPCGSetMaxIter ( HYPRE_Solver solver , HYPRE_Int max_iter );
HYPRE_Int HYPRE_ParCSRPCGSetStopCrit ( HYPRE_Solver solver , HYPRE_Int stop_crit );
HYPRE_Int HYPRE_ParCSRPCGSetTwoNorm ( HYPRE_Solver solver , HYPRE_Int two_norm );
HYPRE_Int HYPRE_ParCSRPCGSetRelChange ( HYPRE_Solver solver , HYPRE_Int rel_change );
HYPRE_Int HYPRE_ParCSRPCGSetPrecond ( HYPRE_Solver solver , HYPRE_PtrToParSolverFcn precond , HYPRE_PtrToParSolverFcn precond_setup , HYPRE_Solver precond_solver );
HYPRE_Int HYPRE_ParCSRPCGGetPrecond ( HYPRE_Solver solver , HYPRE_Solver *precond_data_ptr );
HYPRE_Int HYPRE_ParCSRPCGSetPrintLevel ( HYPRE_Solver solver , HYPRE_Int level );
HYPRE_Int HYPRE_ParCSRPCGSetLogging ( HYPRE_Solver solver , HYPRE_Int level );
HYPRE_Int HYPRE_ParCSRPCGGetNumIterations ( HYPRE_Solver solver , HYPRE_Int *num_iterations );
HYPRE_Int HYPRE_ParCSRPCGGetFinalRelativeResidualNorm ( HYPRE_Solver solver , HYPRE_Real *norm );
HYPRE_Int HYPRE_ParCSRDiagScaleSetup ( HYPRE_Solver solver , HYPRE_ParCSRMatrix A , HYPRE_ParVector y , HYPRE_ParVector x );
HYPRE_Int HYPRE_ParCSRDiagScale ( HYPRE_Solver solver , HYPRE_ParCSRMatrix HA , HYPRE_ParVector Hy , HYPRE_ParVector Hx );

/* par_add_cycle.c */
HYPRE_Int hypre_BoomerAMGAdditiveCycle ( void *amg_vdata );
HYPRE_Int hypre_CreateLambda ( void *amg_vdata );
HYPRE_Int hypre_CreateDinv ( void *amg_vdata );

/* par_amg.c */
void *hypre_BoomerAMGCreate ( void );
HYPRE_Int hypre_BoomerAMGDestroy ( void *data );
HYPRE_Int hypre_BoomerAMGSetRestriction ( void *data , HYPRE_Int restr_par );
HYPRE_Int hypre_BoomerAMGSetMaxLevels ( void *data , HYPRE_Int max_levels );
HYPRE_Int hypre_BoomerAMGGetMaxLevels ( void *data , HYPRE_Int *max_levels );
HYPRE_Int hypre_BoomerAMGSetMaxCoarseSize ( void *data , HYPRE_Int max_coarse_size );
HYPRE_Int hypre_BoomerAMGGetMaxCoarseSize ( void *data , HYPRE_Int *max_coarse_size );
HYPRE_Int hypre_BoomerAMGSetMinCoarseSize ( void *data , HYPRE_Int min_coarse_size );
HYPRE_Int hypre_BoomerAMGGetMinCoarseSize ( void *data , HYPRE_Int *min_coarse_size );
HYPRE_Int hypre_BoomerAMGSetSeqThreshold ( void *data , HYPRE_Int seq_threshold );
HYPRE_Int hypre_BoomerAMGGetSeqThreshold ( void *data , HYPRE_Int *seq_threshold );
HYPRE_Int hypre_BoomerAMGSetRedundant ( void *data , HYPRE_Int redundant );
HYPRE_Int hypre_BoomerAMGGetRedundant ( void *data , HYPRE_Int *redundant );
HYPRE_Int hypre_BoomerAMGSetStrongThreshold ( void *data , HYPRE_Real strong_threshold );
HYPRE_Int hypre_BoomerAMGGetStrongThreshold ( void *data , HYPRE_Real *strong_threshold );
HYPRE_Int hypre_BoomerAMGSetMaxRowSum ( void *data , HYPRE_Real max_row_sum );
HYPRE_Int hypre_BoomerAMGGetMaxRowSum ( void *data , HYPRE_Real *max_row_sum );
HYPRE_Int hypre_BoomerAMGSetTruncFactor ( void *data , HYPRE_Real trunc_factor );
HYPRE_Int hypre_BoomerAMGGetTruncFactor ( void *data , HYPRE_Real *trunc_factor );
HYPRE_Int hypre_BoomerAMGSetPMaxElmts ( void *data , HYPRE_Int P_max_elmts );
HYPRE_Int hypre_BoomerAMGGetPMaxElmts ( void *data , HYPRE_Int *P_max_elmts );
HYPRE_Int hypre_BoomerAMGSetJacobiTruncThreshold ( void *data , HYPRE_Real jacobi_trunc_threshold );
HYPRE_Int hypre_BoomerAMGGetJacobiTruncThreshold ( void *data , HYPRE_Real *jacobi_trunc_threshold );
HYPRE_Int hypre_BoomerAMGSetPostInterpType ( void *data , HYPRE_Int post_interp_type );
HYPRE_Int hypre_BoomerAMGGetPostInterpType ( void *data , HYPRE_Int *post_interp_type );
HYPRE_Int hypre_BoomerAMGSetSCommPkgSwitch ( void *data , HYPRE_Real S_commpkg_switch );
HYPRE_Int hypre_BoomerAMGGetSCommPkgSwitch ( void *data , HYPRE_Real *S_commpkg_switch );
HYPRE_Int hypre_BoomerAMGSetInterpType ( void *data , HYPRE_Int interp_type );
HYPRE_Int hypre_BoomerAMGGetInterpType ( void *data , HYPRE_Int *interp_type );
HYPRE_Int hypre_BoomerAMGSetSepWeight ( void *data , HYPRE_Int sep_weight );
HYPRE_Int hypre_BoomerAMGSetMinIter ( void *data , HYPRE_Int min_iter );
HYPRE_Int hypre_BoomerAMGGetMinIter ( void *data , HYPRE_Int *min_iter );
HYPRE_Int hypre_BoomerAMGSetMaxIter ( void *data , HYPRE_Int max_iter );
HYPRE_Int hypre_BoomerAMGGetMaxIter ( void *data , HYPRE_Int *max_iter );
HYPRE_Int hypre_BoomerAMGSetCoarsenType ( void *data , HYPRE_Int coarsen_type );
HYPRE_Int hypre_BoomerAMGGetCoarsenType ( void *data , HYPRE_Int *coarsen_type );
HYPRE_Int hypre_BoomerAMGSetMeasureType ( void *data , HYPRE_Int measure_type );
HYPRE_Int hypre_BoomerAMGGetMeasureType ( void *data , HYPRE_Int *measure_type );
HYPRE_Int hypre_BoomerAMGSetSetupType ( void *data , HYPRE_Int setup_type );
HYPRE_Int hypre_BoomerAMGGetSetupType ( void *data , HYPRE_Int *setup_type );
HYPRE_Int hypre_BoomerAMGSetCycleType ( void *data , HYPRE_Int cycle_type );
HYPRE_Int hypre_BoomerAMGGetCycleType ( void *data , HYPRE_Int *cycle_type );
HYPRE_Int hypre_BoomerAMGSetTol ( void *data , HYPRE_Real tol );
HYPRE_Int hypre_BoomerAMGGetTol ( void *data , HYPRE_Real *tol );
HYPRE_Int hypre_BoomerAMGSetNumSweeps ( void *data , HYPRE_Int num_sweeps );
HYPRE_Int hypre_BoomerAMGSetCycleNumSweeps ( void *data , HYPRE_Int num_sweeps , HYPRE_Int k );
HYPRE_Int hypre_BoomerAMGGetCycleNumSweeps ( void *data , HYPRE_Int *num_sweeps , HYPRE_Int k );
HYPRE_Int hypre_BoomerAMGSetNumGridSweeps ( void *data , HYPRE_Int *num_grid_sweeps );
HYPRE_Int hypre_BoomerAMGGetNumGridSweeps ( void *data , HYPRE_Int **num_grid_sweeps );
HYPRE_Int hypre_BoomerAMGSetRelaxType ( void *data , HYPRE_Int relax_type );
HYPRE_Int hypre_BoomerAMGSetCycleRelaxType ( void *data , HYPRE_Int relax_type , HYPRE_Int k );
HYPRE_Int hypre_BoomerAMGGetCycleRelaxType ( void *data , HYPRE_Int *relax_type , HYPRE_Int k );
HYPRE_Int hypre_BoomerAMGSetRelaxOrder ( void *data , HYPRE_Int relax_order );
HYPRE_Int hypre_BoomerAMGGetRelaxOrder ( void *data , HYPRE_Int *relax_order );
HYPRE_Int hypre_BoomerAMGSetGridRelaxType ( void *data , HYPRE_Int *grid_relax_type );
HYPRE_Int hypre_BoomerAMGGetGridRelaxType ( void *data , HYPRE_Int **grid_relax_type );
HYPRE_Int hypre_BoomerAMGSetGridRelaxPoints ( void *data , HYPRE_Int **grid_relax_points );
HYPRE_Int hypre_BoomerAMGGetGridRelaxPoints ( void *data , HYPRE_Int ***grid_relax_points );
HYPRE_Int hypre_BoomerAMGSetRelaxWeight ( void *data , HYPRE_Real *relax_weight );
HYPRE_Int hypre_BoomerAMGGetRelaxWeight ( void *data , HYPRE_Real **relax_weight );
HYPRE_Int hypre_BoomerAMGSetRelaxWt ( void *data , HYPRE_Real relax_weight );
HYPRE_Int hypre_BoomerAMGSetLevelRelaxWt ( void *data , HYPRE_Real relax_weight , HYPRE_Int level );
HYPRE_Int hypre_BoomerAMGGetLevelRelaxWt ( void *data , HYPRE_Real *relax_weight , HYPRE_Int level );
HYPRE_Int hypre_BoomerAMGSetOmega ( void *data , HYPRE_Real *omega );
HYPRE_Int hypre_BoomerAMGGetOmega ( void *data , HYPRE_Real **omega );
HYPRE_Int hypre_BoomerAMGSetOuterWt ( void *data , HYPRE_Real omega );
HYPRE_Int hypre_BoomerAMGSetLevelOuterWt ( void *data , HYPRE_Real omega , HYPRE_Int level );
HYPRE_Int hypre_BoomerAMGGetLevelOuterWt ( void *data , HYPRE_Real *omega , HYPRE_Int level );
HYPRE_Int hypre_BoomerAMGSetLogging ( void *data , HYPRE_Int logging );
HYPRE_Int hypre_BoomerAMGGetLogging ( void *data , HYPRE_Int *logging );
HYPRE_Int hypre_BoomerAMGSetPrintLevel ( void *data , HYPRE_Int print_level );
HYPRE_Int hypre_BoomerAMGGetPrintLevel ( void *data , HYPRE_Int *print_level );
HYPRE_Int hypre_BoomerAMGSetPrintFileName ( void *data , const char *print_file_name );
HYPRE_Int hypre_BoomerAMGGetPrintFileName ( void *data , char **print_file_name );
HYPRE_Int hypre_BoomerAMGSetNumIterations ( void *data , HYPRE_Int num_iterations );
HYPRE_Int hypre_BoomerAMGSetDebugFlag ( void *data , HYPRE_Int debug_flag );
HYPRE_Int hypre_BoomerAMGGetDebugFlag ( void *data , HYPRE_Int *debug_flag );
HYPRE_Int hypre_BoomerAMGSetNumFunctions ( void *data , HYPRE_Int num_functions );
HYPRE_Int hypre_BoomerAMGGetNumFunctions ( void *data , HYPRE_Int *num_functions );
HYPRE_Int hypre_BoomerAMGSetNodal ( void *data , HYPRE_Int nodal );
HYPRE_Int hypre_BoomerAMGSetNodalLevels ( void *data , HYPRE_Int nodal_levels );
HYPRE_Int hypre_BoomerAMGSetNodalDiag ( void *data , HYPRE_Int nodal );
HYPRE_Int hypre_BoomerAMGSetNumPaths ( void *data , HYPRE_Int num_paths );
HYPRE_Int hypre_BoomerAMGSetAggNumLevels ( void *data , HYPRE_Int agg_num_levels );
HYPRE_Int hypre_BoomerAMGSetAggInterpType ( void *data , HYPRE_Int agg_interp_type );
HYPRE_Int hypre_BoomerAMGSetAggPMaxElmts ( void *data , HYPRE_Int agg_P_max_elmts );
HYPRE_Int hypre_BoomerAMGSetMultAddPMaxElmts ( void *data , HYPRE_Int add_P_max_elmts );
HYPRE_Int hypre_BoomerAMGSetAddRelaxType ( void *data , HYPRE_Int add_rlx_type );
HYPRE_Int hypre_BoomerAMGSetAddRelaxWt ( void *data , HYPRE_Real add_rlx_wt );
HYPRE_Int hypre_BoomerAMGSetAggP12MaxElmts ( void *data , HYPRE_Int agg_P12_max_elmts );
HYPRE_Int hypre_BoomerAMGSetAggTruncFactor ( void *data , HYPRE_Real agg_trunc_factor );
HYPRE_Int hypre_BoomerAMGSetMultAddTruncFactor ( void *data , HYPRE_Real add_trunc_factor );
HYPRE_Int hypre_BoomerAMGSetAggP12TruncFactor ( void *data , HYPRE_Real agg_P12_trunc_factor );
HYPRE_Int hypre_BoomerAMGSetNumPoints ( void *data , HYPRE_Int num_points );
HYPRE_Int hypre_BoomerAMGSetDofFunc ( void *data , HYPRE_Int *dof_func );
HYPRE_Int hypre_BoomerAMGSetPointDofMap ( void *data , HYPRE_Int *point_dof_map );
HYPRE_Int hypre_BoomerAMGSetDofPoint ( void *data , HYPRE_Int *dof_point );
HYPRE_Int hypre_BoomerAMGGetNumIterations ( void *data , HYPRE_Int *num_iterations );
HYPRE_Int hypre_BoomerAMGGetCumNumIterations ( void *data , HYPRE_Int *cum_num_iterations );
HYPRE_Int hypre_BoomerAMGGetCumNnzAP ( void *data , HYPRE_Real *cum_nnz_AP );
HYPRE_Int hypre_BoomerAMGGetResidual ( void *data , hypre_ParVector **resid );
HYPRE_Int hypre_BoomerAMGGetRelResidualNorm ( void *data , HYPRE_Real *rel_resid_norm );
HYPRE_Int hypre_BoomerAMGSetChebyOrder ( void *data , HYPRE_Int order );
HYPRE_Int hypre_BoomerAMGSetChebyFraction ( void *data , HYPRE_Real ratio );
HYPRE_Int hypre_BoomerAMGSetChebyEigEst ( void *data , HYPRE_Int eig_est );
HYPRE_Int hypre_BoomerAMGSetChebyVariant ( void *data , HYPRE_Int variant );
HYPRE_Int hypre_BoomerAMGSetChebyScale ( void *data , HYPRE_Int scale );
HYPRE_Int hypre_BoomerAMGSetAdditive ( void *data , HYPRE_Int additive );
HYPRE_Int hypre_BoomerAMGGetAdditive ( void *data , HYPRE_Int *additive );
HYPRE_Int hypre_BoomerAMGSetMultAdditive ( void *data , HYPRE_Int mult_additive );
HYPRE_Int hypre_BoomerAMGGetMultAdditive ( void *data , HYPRE_Int *mult_additive );
HYPRE_Int hypre_BoomerAMGSetSimple ( void *data , HYPRE_Int simple );
HYPRE_Int hypre_BoomerAMGGetSimple ( void *data , HYPRE_Int *simple );
HYPRE_Int hypre_BoomerAMGSetAddLastLvl ( void *data , HYPRE_Int add_last_lvl );
HYPRE_Int hypre_BoomerAMGSetNonGalerkinTol ( void *data , HYPRE_Real nongalerkin_tol );
HYPRE_Int hypre_BoomerAMGSetLevelNonGalerkinTol ( void *data , HYPRE_Real nongalerkin_tol , HYPRE_Int level );
HYPRE_Int hypre_BoomerAMGSetNonGalerkTol ( void *data , HYPRE_Int nongalerk_num_tol , HYPRE_Real *nongalerk_tol );
HYPRE_Int hypre_BoomerAMGSetRAP2 ( void *data , HYPRE_Int rap2 );
HYPRE_Int hypre_BoomerAMGSetKeepTranspose ( void *data , HYPRE_Int keepTranspose );

/* par_amg_setup.c */
HYPRE_Int hypre_BoomerAMGSetup ( void *amg_vdata , hypre_ParCSRMatrix *A , hypre_ParVector *f , hypre_ParVector *u );

/* par_amg_solve.c */
HYPRE_Int hypre_BoomerAMGSolve ( void *amg_vdata , hypre_ParCSRMatrix *A , hypre_ParVector *f , hypre_ParVector *u );

/* par_cg_relax_wt.c */
HYPRE_Int hypre_BoomerAMGCGRelaxWt ( void *amg_vdata , HYPRE_Int level , HYPRE_Int num_cg_sweeps , HYPRE_Real *rlx_wt_ptr );
HYPRE_Int hypre_Bisection ( HYPRE_Int n , HYPRE_Real *diag , HYPRE_Real *offd , HYPRE_Real y , HYPRE_Real z , HYPRE_Real tol , HYPRE_Int k , HYPRE_Real *ev_ptr );

/* par_cheby.c */
HYPRE_Int hypre_ParCSRRelax_Cheby_Setup ( hypre_ParCSRMatrix *A , HYPRE_Real max_eig , HYPRE_Real min_eig , HYPRE_Real fraction , HYPRE_Int order , HYPRE_Int scale , HYPRE_Int variant , HYPRE_Real **coefs_ptr , HYPRE_Real **ds_ptr );
HYPRE_Int hypre_ParCSRRelax_Cheby_Solve ( hypre_ParCSRMatrix *A , hypre_ParVector *f , HYPRE_Real *ds_data , HYPRE_Real *coefs , HYPRE_Int order , HYPRE_Int scale , HYPRE_Int variant , hypre_ParVector *u , hypre_ParVector *v , hypre_ParVector *r );

/* par_coarsen.c */
HYPRE_Int hypre_BoomerAMGCoarsen ( hypre_ParCSRMatrix *S , hypre_ParCSRMatrix *A , HYPRE_Int CF_init , HYPRE_Int debug_flag , HYPRE_Int **CF_marker_ptr );
HYPRE_Int hypre_BoomerAMGCoarsenRuge ( hypre_ParCSRMatrix *S , hypre_ParCSRMatrix *A , HYPRE_Int measure_type , HYPRE_Int coarsen_type , HYPRE_Int debug_flag , HYPRE_Int **CF_marker_ptr );
HYPRE_Int hypre_BoomerAMGCoarsenFalgout ( hypre_ParCSRMatrix *S , hypre_ParCSRMatrix *A , HYPRE_Int measure_type , HYPRE_Int debug_flag , HYPRE_Int **CF_marker_ptr );
HYPRE_Int hypre_BoomerAMGCoarsenHMIS ( hypre_ParCSRMatrix *S , hypre_ParCSRMatrix *A , HYPRE_Int measure_type , HYPRE_Int debug_flag , HYPRE_Int **CF_marker_ptr );
HYPRE_Int hypre_BoomerAMGCoarsenPMIS ( hypre_ParCSRMatrix *S , hypre_ParCSRMatrix *A , HYPRE_Int CF_init , HYPRE_Int debug_flag , HYPRE_Int **CF_marker_ptr );

/* par_coarse_parms.c */
HYPRE_Int hypre_BoomerAMGCoarseParms ( MPI_Comm comm , HYPRE_Int local_num_variables , HYPRE_Int num_functions , HYPRE_Int *dof_func , HYPRE_Int *CF_marker , HYPRE_Int **coarse_dof_func_ptr , HYPRE_Int **coarse_pnts_global_ptr );

/* par_cycle.c */
HYPRE_Int hypre_BoomerAMGCycle ( void *amg_vdata , hypre_ParVector **F_array , hypre_ParVector **U_array );

/* par_difconv.c */
HYPRE_ParCSRMatrix GenerateDifConv ( MPI_Comm comm , HYPRE_Int nx , HYPRE_Int ny , HYPRE_Int nz , HYPRE_Int P , HYPRE_Int Q , HYPRE_Int R , HYPRE_Int p , HYPRE_Int q , HYPRE_Int r , HYPRE_Real *value );

/* par_indepset.c */
HYPRE_Int hypre_BoomerAMGIndepSetInit ( hypre_ParCSRMatrix *S , HYPRE_Real *measure_array , HYPRE_Int seq_rand );
HYPRE_Int hypre_BoomerAMGIndepSet ( hypre_ParCSRMatrix *S , HYPRE_Real *measure_array , HYPRE_Int *graph_array , HYPRE_Int graph_array_size , HYPRE_Int *graph_array_offd , HYPRE_Int graph_array_offd_size , HYPRE_Int *IS_marker , HYPRE_Int *IS_marker_offd );

/* par_interp.c */
HYPRE_Int hypre_BoomerAMGBuildInterp ( hypre_ParCSRMatrix *A , HYPRE_Int *CF_marker , hypre_ParCSRMatrix *S , HYPRE_Int *num_cpts_global , HYPRE_Int num_functions , HYPRE_Int *dof_func , HYPRE_Int debug_flag , HYPRE_Real trunc_factor , HYPRE_Int max_elmts , HYPRE_Int *col_offd_S_to_A , hypre_ParCSRMatrix **P_ptr );
HYPRE_Int hypre_BoomerAMGBuildInterpHE ( hypre_ParCSRMatrix *A , HYPRE_Int *CF_marker , hypre_ParCSRMatrix *S , HYPRE_Int *num_cpts_global , HYPRE_Int num_functions , HYPRE_Int *dof_func , HYPRE_Int debug_flag , HYPRE_Real trunc_factor , HYPRE_Int max_elmts , HYPRE_Int *col_offd_S_to_A , hypre_ParCSRMatrix **P_ptr );
HYPRE_Int hypre_BoomerAMGBuildDirInterp ( hypre_ParCSRMatrix *A , HYPRE_Int *CF_marker , hypre_ParCSRMatrix *S , HYPRE_Int *num_cpts_global , HYPRE_Int num_functions , HYPRE_Int *dof_func , HYPRE_Int debug_flag , HYPRE_Real trunc_factor , HYPRE_Int max_elmts , HYPRE_Int *col_offd_S_to_A , hypre_ParCSRMatrix **P_ptr );
HYPRE_Int hypre_BoomerAMGInterpTruncation ( hypre_ParCSRMatrix *P , HYPRE_Real trunc_factor , HYPRE_Int max_elmts );
void hypre_qsort2abs ( HYPRE_Int *v , HYPRE_Real *w , HYPRE_Int left , HYPRE_Int right );
HYPRE_Int hypre_BoomerAMGBuildInterpModUnk ( hypre_ParCSRMatrix *A , HYPRE_Int *CF_marker , hypre_ParCSRMatrix *S , HYPRE_Int *num_cpts_global , HYPRE_Int num_functions , HYPRE_Int *dof_func , HYPRE_Int debug_flag , HYPRE_Real trunc_factor , HYPRE_Int max_elmts , HYPRE_Int *col_offd_S_to_A , hypre_ParCSRMatrix **P_ptr );
HYPRE_Int hypre_BoomerAMGTruncandBuild ( hypre_ParCSRMatrix *P , HYPRE_Real trunc_factor , HYPRE_Int max_elmts );
hypre_ParCSRMatrix *hypre_CreateC ( hypre_ParCSRMatrix *A , HYPRE_Real w );

/* par_jacobi_interp.c */
void hypre_BoomerAMGJacobiInterp ( hypre_ParCSRMatrix *A , hypre_ParCSRMatrix **P , hypre_ParCSRMatrix *S , HYPRE_Int num_functions , HYPRE_Int *dof_func , HYPRE_Int *CF_marker , HYPRE_Int level , HYPRE_Real truncation_threshold , HYPRE_Real truncation_threshold_minus );
void hypre_BoomerAMGJacobiInterp_1 ( hypre_ParCSRMatrix *A , hypre_ParCSRMatrix **P , hypre_ParCSRMatrix *S , HYPRE_Int *CF_marker , HYPRE_Int level , HYPRE_Real truncation_threshold , HYPRE_Real truncation_threshold_minus , HYPRE_Int *dof_func , HYPRE_Int *dof_func_offd , HYPRE_Real weight_AF );
void hypre_BoomerAMGTruncateInterp ( hypre_ParCSRMatrix *P , HYPRE_Real eps , HYPRE_Real dlt , HYPRE_Int *CF_marker );
HYPRE_Int hypre_ParCSRMatrix_dof_func_offd ( hypre_ParCSRMatrix *A , HYPRE_Int num_functions , HYPRE_Int *dof_func , HYPRE_Int **dof_func_offd );

/* par_laplace_27pt.c */
HYPRE_ParCSRMatrix GenerateLaplacian27pt ( MPI_Comm comm , HYPRE_Int nx , HYPRE_Int ny , HYPRE_Int nz , HYPRE_Int P , HYPRE_Int Q , HYPRE_Int R , HYPRE_Int p , HYPRE_Int q , HYPRE_Int r , HYPRE_Real *value );
HYPRE_Int hypre_map3 ( HYPRE_Int ix , HYPRE_Int iy , HYPRE_Int iz , HYPRE_Int p , HYPRE_Int q , HYPRE_Int r , HYPRE_Int P , HYPRE_Int Q , HYPRE_Int R , HYPRE_Int *nx_part , HYPRE_Int *ny_part , HYPRE_Int *nz_part , HYPRE_Int *global_part );

/* par_laplace_9pt.c */
HYPRE_ParCSRMatrix GenerateLaplacian9pt ( MPI_Comm comm , HYPRE_Int nx , HYPRE_Int ny , HYPRE_Int P , HYPRE_Int Q , HYPRE_Int p , HYPRE_Int q , HYPRE_Real *value );
HYPRE_Int hypre_map2 ( HYPRE_Int ix , HYPRE_Int iy , HYPRE_Int p , HYPRE_Int q , HYPRE_Int P , HYPRE_Int Q , HYPRE_Int *nx_part , HYPRE_Int *ny_part , HYPRE_Int *global_part );

/* par_laplace.c */
HYPRE_ParCSRMatrix GenerateLaplacian ( MPI_Comm comm , HYPRE_Int nx , HYPRE_Int ny , HYPRE_Int nz , HYPRE_Int P , HYPRE_Int Q , HYPRE_Int R , HYPRE_Int p , HYPRE_Int q , HYPRE_Int r , HYPRE_Real *value );
HYPRE_Int hypre_map ( HYPRE_Int ix , HYPRE_Int iy , HYPRE_Int iz , HYPRE_Int p , HYPRE_Int q , HYPRE_Int r , HYPRE_Int P , HYPRE_Int Q , HYPRE_Int R , HYPRE_Int *nx_part , HYPRE_Int *ny_part , HYPRE_Int *nz_part , HYPRE_Int *global_part );
HYPRE_ParCSRMatrix GenerateSysLaplacian ( MPI_Comm comm , HYPRE_Int nx , HYPRE_Int ny , HYPRE_Int nz , HYPRE_Int P , HYPRE_Int Q , HYPRE_Int R , HYPRE_Int p , HYPRE_Int q , HYPRE_Int r , HYPRE_Int num_fun , HYPRE_Real *mtrx , HYPRE_Real *value );
HYPRE_ParCSRMatrix GenerateSysLaplacianVCoef ( MPI_Comm comm , HYPRE_Int nx , HYPRE_Int ny , HYPRE_Int nz , HYPRE_Int P , HYPRE_Int Q , HYPRE_Int R , HYPRE_Int p , HYPRE_Int q , HYPRE_Int r , HYPRE_Int num_fun , HYPRE_Real *mtrx , HYPRE_Real *value );

/* par_lr_interp.c */
HYPRE_Int hypre_BoomerAMGBuildStdInterp ( hypre_ParCSRMatrix *A , HYPRE_Int *CF_marker , hypre_ParCSRMatrix *S , HYPRE_Int *num_cpts_global , HYPRE_Int num_functions , HYPRE_Int *dof_func , HYPRE_Int debug_flag , HYPRE_Real trunc_factor , HYPRE_Int max_elmts , HYPRE_Int sep_weight , HYPRE_Int *col_offd_S_to_A , hypre_ParCSRMatrix **P_ptr );
HYPRE_Int hypre_BoomerAMGBuildExtPIInterp ( hypre_ParCSRMatrix *A , HYPRE_Int *CF_marker , hypre_ParCSRMatrix *S , HYPRE_Int *num_cpts_global , HYPRE_Int num_functions , HYPRE_Int *dof_func , HYPRE_Int debug_flag , HYPRE_Real trunc_factor , HYPRE_Int max_elmts , HYPRE_Int *col_offd_S_to_A , hypre_ParCSRMatrix **P_ptr );
HYPRE_Int hypre_BoomerAMGBuildExtPICCInterp ( hypre_ParCSRMatrix *A , HYPRE_Int *CF_marker , hypre_ParCSRMatrix *S , HYPRE_Int *num_cpts_global , HYPRE_Int num_functions , HYPRE_Int *dof_func , HYPRE_Int debug_flag , HYPRE_Real trunc_factor , HYPRE_Int max_elmts , HYPRE_Int *col_offd_S_to_A , hypre_ParCSRMatrix **P_ptr );
HYPRE_Int hypre_BoomerAMGBuildFFInterp ( hypre_ParCSRMatrix *A , HYPRE_Int *CF_marker , hypre_ParCSRMatrix *S , HYPRE_Int *num_cpts_global , HYPRE_Int num_functions , HYPRE_Int *dof_func , HYPRE_Int debug_flag , HYPRE_Real trunc_factor , HYPRE_Int max_elmts , HYPRE_Int *col_offd_S_to_A , hypre_ParCSRMatrix **P_ptr );
HYPRE_Int hypre_BoomerAMGBuildFF1Interp ( hypre_ParCSRMatrix *A , HYPRE_Int *CF_marker , hypre_ParCSRMatrix *S , HYPRE_Int *num_cpts_global , HYPRE_Int num_functions , HYPRE_Int *dof_func , HYPRE_Int debug_flag , HYPRE_Real trunc_factor , HYPRE_Int max_elmts , HYPRE_Int *col_offd_S_to_A , hypre_ParCSRMatrix **P_ptr );
HYPRE_Int hypre_BoomerAMGBuildExtInterp ( hypre_ParCSRMatrix *A , HYPRE_Int *CF_marker , hypre_ParCSRMatrix *S , HYPRE_Int *num_cpts_global , HYPRE_Int num_functions , HYPRE_Int *dof_func , HYPRE_Int debug_flag , HYPRE_Real trunc_factor , HYPRE_Int max_elmts , HYPRE_Int *col_offd_S_to_A , hypre_ParCSRMatrix **P_ptr );

/* par_multi_interp.c */
HYPRE_Int hypre_BoomerAMGBuildMultipass ( hypre_ParCSRMatrix *A , HYPRE_Int *CF_marker , hypre_ParCSRMatrix *S , HYPRE_Int *num_cpts_global , HYPRE_Int num_functions , HYPRE_Int *dof_func , HYPRE_Int debug_flag , HYPRE_Real trunc_factor , HYPRE_Int P_max_elmts , HYPRE_Int weight_option , HYPRE_Int *col_offd_S_to_A , hypre_ParCSRMatrix **P_ptr );

/* par_nongalerkin.c */
HYPRE_Int hypre_GrabSubArray ( HYPRE_Int *indices , HYPRE_Int start , HYPRE_Int end , HYPRE_Int *array , HYPRE_Int *output );
void hypre_qsort2_abs ( HYPRE_Int *v , HYPRE_Real *w , HYPRE_Int left , HYPRE_Int right );
HYPRE_Int hypre_IntersectTwoArrays ( HYPRE_Int *x , HYPRE_Real *x_data , HYPRE_Int x_length , HYPRE_Int *y , HYPRE_Int y_length , HYPRE_Int *z , HYPRE_Real *output_x_data , HYPRE_Int *intersect_length );
HYPRE_Int hypre_SortedCopyParCSRData ( hypre_ParCSRMatrix *A , hypre_ParCSRMatrix *B );
HYPRE_Int hypre_BoomerAMG_MyCreateS ( hypre_ParCSRMatrix *A , HYPRE_Real strength_threshold , HYPRE_Real max_row_sum , HYPRE_Int num_functions , HYPRE_Int *dof_func , hypre_ParCSRMatrix **S_ptr );
HYPRE_Int hypre_NonGalerkinIJBufferInit ( HYPRE_Int *ijbuf_cnt , HYPRE_Int *ijbuf_rowcounter , HYPRE_Int *ijbuf_numcols );
HYPRE_Int hypre_NonGalerkinIJBufferNewRow ( HYPRE_Int *ijbuf_rownums , HYPRE_Int *ijbuf_numcols , HYPRE_Int *ijbuf_rowcounter , HYPRE_Int new_row );
HYPRE_Int hypre_NonGalerkinIJBufferCompressRow ( HYPRE_Int *ijbuf_cnt , HYPRE_Int ijbuf_rowcounter , HYPRE_Real *ijbuf_data , HYPRE_Int *ijbuf_cols , HYPRE_Int *ijbuf_rownums , HYPRE_Int *ijbuf_numcols );
HYPRE_Int hypre_NonGalerkinIJBufferCompress ( HYPRE_Int ijbuf_size , HYPRE_Int *ijbuf_cnt , HYPRE_Int *ijbuf_rowcounter , HYPRE_Real **ijbuf_data , HYPRE_Int **ijbuf_cols , HYPRE_Int **ijbuf_rownums , HYPRE_Int **ijbuf_numcols );
HYPRE_Int hypre_NonGalerkinIJBufferWrite ( HYPRE_IJMatrix B , HYPRE_Int *ijbuf_cnt , HYPRE_Int ijbuf_size , HYPRE_Int *ijbuf_rowcounter , HYPRE_Real **ijbuf_data , HYPRE_Int **ijbuf_cols , HYPRE_Int **ijbuf_rownums , HYPRE_Int **ijbuf_numcols , HYPRE_Int row_to_write , HYPRE_Int col_to_write , HYPRE_Real val_to_write );
HYPRE_Int hypre_NonGalerkinIJBufferEmpty ( HYPRE_IJMatrix B , HYPRE_Int ijbuf_size , HYPRE_Int *ijbuf_cnt , HYPRE_Int ijbuf_rowcounter , HYPRE_Real **ijbuf_data , HYPRE_Int **ijbuf_cols , HYPRE_Int **ijbuf_rownums , HYPRE_Int **ijbuf_numcols );
hypre_ParCSRMatrix * hypre_NonGalerkinSparsityPattern(hypre_ParCSRMatrix *R_IAP, hypre_ParCSRMatrix *RAP, HYPRE_Int * CF_marker, HYPRE_Real droptol, HYPRE_Int sym_collapse, HYPRE_Int collapse_beta );
HYPRE_Int hypre_BoomerAMGBuildNonGalerkinCoarseOperator( hypre_ParCSRMatrix **RAP_ptr, hypre_ParCSRMatrix *AP, HYPRE_Real strong_threshold, HYPRE_Real max_row_sum, HYPRE_Int num_functions, HYPRE_Int * dof_func_value, HYPRE_Real S_commpkg_switch, HYPRE_Int * CF_marker, HYPRE_Real droptol, HYPRE_Int sym_collapse, HYPRE_Real lump_percent, HYPRE_Int collapse_beta );

/* par_rap.c */
hypre_CSRMatrix *hypre_ExchangeRAPData ( hypre_CSRMatrix *RAP_int , hypre_ParCSRCommPkg *comm_pkg_RT );
HYPRE_Int hypre_BoomerAMGBuildCoarseOperator ( hypre_ParCSRMatrix *RT , hypre_ParCSRMatrix *A , hypre_ParCSRMatrix *P , hypre_ParCSRMatrix **RAP_ptr );
HYPRE_Int hypre_BoomerAMGBuildCoarseOperatorKT ( hypre_ParCSRMatrix *RT , hypre_ParCSRMatrix *A , hypre_ParCSRMatrix *P , HYPRE_Int keepTranspose, hypre_ParCSRMatrix **RAP_ptr );

/* par_rap_communication.c */
HYPRE_Int hypre_GetCommPkgRTFromCommPkgA ( hypre_ParCSRMatrix *RT , hypre_ParCSRMatrix *A , HYPRE_Int *fine_to_coarse_offd );
HYPRE_Int hypre_GenerateSendMapAndCommPkg ( MPI_Comm comm , HYPRE_Int num_sends , HYPRE_Int num_recvs , HYPRE_Int *recv_procs , HYPRE_Int *send_procs , HYPRE_Int *recv_vec_starts , hypre_ParCSRMatrix *A );

/* par_relax.c */
HYPRE_Int hypre_BoomerAMGRelax ( hypre_ParCSRMatrix *A , hypre_ParVector *f , HYPRE_Int *cf_marker , HYPRE_Int relax_type , HYPRE_Int relax_points , HYPRE_Real relax_weight , HYPRE_Real omega , HYPRE_Real *l1_norms , hypre_ParVector *u , hypre_ParVector *Vtemp , hypre_ParVector *Ztemp );
HYPRE_Int hypre_GaussElimSetup ( hypre_ParAMGData *amg_data , HYPRE_Int level , HYPRE_Int relax_type );
HYPRE_Int hypre_GaussElimSolve ( hypre_ParAMGData *amg_data , HYPRE_Int level , HYPRE_Int relax_type );
HYPRE_Int gselim ( HYPRE_Real *A , HYPRE_Real *x , HYPRE_Int n );

/* par_relax_interface.c */
HYPRE_Int hypre_BoomerAMGRelaxIF ( hypre_ParCSRMatrix *A , hypre_ParVector *f , HYPRE_Int *cf_marker , HYPRE_Int relax_type , HYPRE_Int relax_order , HYPRE_Int cycle_type , HYPRE_Real relax_weight , HYPRE_Real omega , HYPRE_Real *l1_norms , hypre_ParVector *u , hypre_ParVector *Vtemp , hypre_ParVector *Ztemp );

/* par_relax_more.c */
HYPRE_Int hypre_ParCSRMaxEigEstimate ( hypre_ParCSRMatrix *A , HYPRE_Int scale , HYPRE_Real *max_eig );
HYPRE_Int hypre_ParCSRMaxEigEstimateCG ( hypre_ParCSRMatrix *A , HYPRE_Int scale , HYPRE_Int max_iter , HYPRE_Real *max_eig , HYPRE_Real *min_eig );
HYPRE_Int hypre_ParCSRRelax_Cheby ( hypre_ParCSRMatrix *A , hypre_ParVector *f , HYPRE_Real max_eig , HYPRE_Real min_eig , HYPRE_Real fraction , HYPRE_Int order , HYPRE_Int scale , HYPRE_Int variant , hypre_ParVector *u , hypre_ParVector *v , hypre_ParVector *r );
HYPRE_Int hypre_BoomerAMGRelax_FCFJacobi ( hypre_ParCSRMatrix *A , hypre_ParVector *f , HYPRE_Int *cf_marker , HYPRE_Real relax_weight , hypre_ParVector *u , hypre_ParVector *Vtemp );
HYPRE_Int hypre_ParCSRRelax_CG ( HYPRE_Solver solver , hypre_ParCSRMatrix *A , hypre_ParVector *f , hypre_ParVector *u , HYPRE_Int num_its );
HYPRE_Int hypre_LINPACKcgtql1 ( HYPRE_Int *n , HYPRE_Real *d , HYPRE_Real *e , HYPRE_Int *ierr );
HYPRE_Real hypre_LINPACKcgpthy ( HYPRE_Real *a , HYPRE_Real *b );
HYPRE_Int hypre_ParCSRRelax_L1_Jacobi ( hypre_ParCSRMatrix *A , hypre_ParVector *f , HYPRE_Int *cf_marker , HYPRE_Int relax_points , HYPRE_Real relax_weight , HYPRE_Real *l1_norms , hypre_ParVector *u , hypre_ParVector *Vtemp );

/* par_rotate_7pt.c */
HYPRE_ParCSRMatrix GenerateRotate7pt ( MPI_Comm comm , HYPRE_Int nx , HYPRE_Int ny , HYPRE_Int P , HYPRE_Int Q , HYPRE_Int p , HYPRE_Int q , HYPRE_Real alpha , HYPRE_Real eps );

/* par_scaled_matnorm.c */
HYPRE_Int hypre_ParCSRMatrixScaledNorm ( hypre_ParCSRMatrix *A , HYPRE_Real *scnorm );

/* par_stats.c */
HYPRE_Int hypre_BoomerAMGSetupStats ( void *amg_vdata , hypre_ParCSRMatrix *A );
HYPRE_Int hypre_BoomerAMGWriteSolverParams ( void *data );

/* par_strength.c */
HYPRE_Int hypre_BoomerAMGCreateS ( hypre_ParCSRMatrix *A , HYPRE_Real strength_threshold , HYPRE_Real max_row_sum , HYPRE_Int num_functions , HYPRE_Int *dof_func , hypre_ParCSRMatrix **S_ptr );
HYPRE_Int hypre_BoomerAMGCreateSabs ( hypre_ParCSRMatrix *A , HYPRE_Real strength_threshold , HYPRE_Real max_row_sum , HYPRE_Int num_functions , HYPRE_Int *dof_func , hypre_ParCSRMatrix **S_ptr );
HYPRE_Int hypre_BoomerAMGCreateSCommPkg ( hypre_ParCSRMatrix *A , hypre_ParCSRMatrix *S , HYPRE_Int **col_offd_S_to_A_ptr );
HYPRE_Int hypre_BoomerAMGCreate2ndS ( hypre_ParCSRMatrix *S , HYPRE_Int *CF_marker , HYPRE_Int num_paths , HYPRE_Int *coarse_row_starts , hypre_ParCSRMatrix **C_ptr );
HYPRE_Int hypre_BoomerAMGCorrectCFMarker ( HYPRE_Int *CF_marker , HYPRE_Int num_var , HYPRE_Int *new_CF_marker );
HYPRE_Int hypre_BoomerAMGCorrectCFMarker2 ( HYPRE_Int *CF_marker , HYPRE_Int num_var , HYPRE_Int *new_CF_marker );

/* partial.c */
HYPRE_Int hypre_BoomerAMGBuildPartialExtPIInterp ( hypre_ParCSRMatrix *A , HYPRE_Int *CF_marker , hypre_ParCSRMatrix *S , HYPRE_Int *num_cpts_global , HYPRE_Int *num_old_cpts_global , HYPRE_Int num_functions , HYPRE_Int *dof_func , HYPRE_Int debug_flag , HYPRE_Real trunc_factor , HYPRE_Int max_elmts , HYPRE_Int *col_offd_S_to_A , hypre_ParCSRMatrix **P_ptr );
HYPRE_Int hypre_BoomerAMGBuildPartialStdInterp ( hypre_ParCSRMatrix *A , HYPRE_Int *CF_marker , hypre_ParCSRMatrix *S , HYPRE_Int *num_cpts_global , HYPRE_Int *num_old_cpts_global , HYPRE_Int num_functions , HYPRE_Int *dof_func , HYPRE_Int debug_flag , HYPRE_Real trunc_factor , HYPRE_Int max_elmts , HYPRE_Int sep_weight , HYPRE_Int *col_offd_S_to_A , hypre_ParCSRMatrix **P_ptr );
HYPRE_Int hypre_BoomerAMGBuildPartialExtInterp ( hypre_ParCSRMatrix *A , HYPRE_Int *CF_marker , hypre_ParCSRMatrix *S , HYPRE_Int *num_cpts_global , HYPRE_Int *num_old_cpts_global , HYPRE_Int num_functions , HYPRE_Int *dof_func , HYPRE_Int debug_flag , HYPRE_Real trunc_factor , HYPRE_Int max_elmts , HYPRE_Int *col_offd_S_to_A , hypre_ParCSRMatrix **P_ptr );

/* par_vardifconv.c */
HYPRE_ParCSRMatrix GenerateVarDifConv ( MPI_Comm comm , HYPRE_Int nx , HYPRE_Int ny , HYPRE_Int nz , HYPRE_Int P , HYPRE_Int Q , HYPRE_Int R , HYPRE_Int p , HYPRE_Int q , HYPRE_Int r , HYPRE_Real eps , HYPRE_ParVector *rhs_ptr );
HYPRE_Real afun ( HYPRE_Real xx , HYPRE_Real yy , HYPRE_Real zz );
HYPRE_Real bfun ( HYPRE_Real xx , HYPRE_Real yy , HYPRE_Real zz );
HYPRE_Real cfun ( HYPRE_Real xx , HYPRE_Real yy , HYPRE_Real zz );
HYPRE_Real dfun ( HYPRE_Real xx , HYPRE_Real yy , HYPRE_Real zz );
HYPRE_Real efun ( HYPRE_Real xx , HYPRE_Real yy , HYPRE_Real zz );
HYPRE_Real ffun ( HYPRE_Real xx , HYPRE_Real yy , HYPRE_Real zz );
HYPRE_Real gfun ( HYPRE_Real xx , HYPRE_Real yy , HYPRE_Real zz );
HYPRE_Real rfun ( HYPRE_Real xx , HYPRE_Real yy , HYPRE_Real zz );
HYPRE_Real bndfun ( HYPRE_Real xx , HYPRE_Real yy , HYPRE_Real zz );

/* pcg_par.c */
char *hypre_ParKrylovCAlloc ( HYPRE_Int count , HYPRE_Int elt_size );
HYPRE_Int hypre_ParKrylovFree ( char *ptr );
void *hypre_ParKrylovCreateVector ( void *vvector );
void *hypre_ParKrylovCreateVectorArray ( HYPRE_Int n , void *vvector );
HYPRE_Int hypre_ParKrylovDestroyVector ( void *vvector );
void *hypre_ParKrylovMatvecCreate ( void *A , void *x );
HYPRE_Int hypre_ParKrylovMatvec ( void *matvec_data , HYPRE_Complex alpha , void *A , void *x , HYPRE_Complex beta , void *y );
HYPRE_Int hypre_ParKrylovMatvecT ( void *matvec_data , HYPRE_Complex alpha , void *A , void *x , HYPRE_Complex beta , void *y );
HYPRE_Int hypre_ParKrylovMatvecDestroy ( void *matvec_data );
HYPRE_Real hypre_ParKrylovInnerProd ( void *x , void *y );
HYPRE_Int hypre_ParKrylovCopyVector ( void *x , void *y );
HYPRE_Int hypre_ParKrylovClearVector ( void *x );
HYPRE_Int hypre_ParKrylovScaleVector ( HYPRE_Complex alpha , void *x );
HYPRE_Int hypre_ParKrylovAxpy ( HYPRE_Complex alpha , void *x , void *y );
HYPRE_Int hypre_ParKrylovCommInfo ( void *A , HYPRE_Int *my_id , HYPRE_Int *num_procs );
HYPRE_Int hypre_ParKrylovIdentitySetup ( void *vdata , void *A , void *b , void *x );
HYPRE_Int hypre_ParKrylovIdentity ( void *vdata , void *A , void *b , void *x );

#ifdef __cplusplus
}
#endif

#endif

