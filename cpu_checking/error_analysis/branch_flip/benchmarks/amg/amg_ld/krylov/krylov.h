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

#include "HYPRE_krylov.h"

#ifndef hypre_KRYLOV_HEADER
#define hypre_KRYLOV_HEADER

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "_hypre_utilities.h"

#define hypre_CTAllocF(type, count, funcs) ( (type *)(*(funcs->CAlloc))((size_t)(count), (size_t)sizeof(type)) )

#define hypre_TFreeF( ptr, funcs ) ( (*(funcs->Free))((char *)ptr), ptr = NULL )

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
 *
 * GMRES gmres
 *
 *****************************************************************************/

#ifndef hypre_KRYLOV_GMRES_HEADER
#define hypre_KRYLOV_GMRES_HEADER

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

/**
 * @name Generic GMRES Interface
 *
 * A general description of the interface goes here...
 *
 * @memo A generic GMRES linear solver interface
 * @version 0.1
 * @author Jeffrey F. Painter
 **/
/*@{*/

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
 * hypre_GMRESData and hypre_GMRESFunctions
 *--------------------------------------------------------------------------*/

/**
 * @name GMRES structs
 *
 * Description...
 **/
/*@{*/

/**
 * The {\tt hypre\_GMRESFunctions} object ...
 **/

typedef struct
{
   char *       (*CAlloc)        ( size_t count, size_t elt_size );
   HYPRE_Int    (*Free)          ( char *ptr );
   HYPRE_Int    (*CommInfo)      ( void  *A, HYPRE_Int   *my_id,
                                   HYPRE_Int   *num_procs );
   void *       (*CreateVector)  ( void *vector );
   void *       (*CreateVectorArray)  ( HYPRE_Int size, void *vectors );
   HYPRE_Int    (*DestroyVector) ( void *vector );
   void *       (*MatvecCreate)  ( void *A, void *x );
   HYPRE_Int    (*Matvec)        ( void *matvec_data, HYPRE_Complex alpha, void *A,
                                   void *x, HYPRE_Complex beta, void *y );
   HYPRE_Int    (*MatvecDestroy) ( void *matvec_data );
   HYPRE_Real   (*InnerProd)     ( void *x, void *y );
   HYPRE_Int    (*CopyVector)    ( void *x, void *y );
   HYPRE_Int    (*ClearVector)   ( void *x );
   HYPRE_Int    (*ScaleVector)   ( HYPRE_Complex alpha, void *x );
   HYPRE_Int    (*Axpy)          ( HYPRE_Complex alpha, void *x, void *y );

	HYPRE_Int    (*precond)       (void *vdata , void *A , void *b , void *x);
	HYPRE_Int    (*precond_setup) (void *vdata , void *A , void *b , void *x);

} hypre_GMRESFunctions;

/**
 * The {\tt hypre\_GMRESData} object ...
 **/

typedef struct
{
   HYPRE_Int      k_dim;
   HYPRE_Int      min_iter;
   HYPRE_Int      max_iter;
   HYPRE_Int      rel_change;
   HYPRE_Int      skip_real_r_check;
   HYPRE_Int      stop_crit;
   HYPRE_Int      converged;
   HYPRE_Real   tol;
   HYPRE_Real   cf_tol;
   HYPRE_Real   a_tol;
   HYPRE_Real   rel_residual_norm;

   void  *A;
   void  *r;
   void  *w;
   void  *w_2;
   void  **p;

   void    *matvec_data;
   void    *precond_data;

   hypre_GMRESFunctions * functions;

   /* log info (always logged) */
   HYPRE_Int      num_iterations;
 
   HYPRE_Int     print_level; /* printing when print_level>0 */
   HYPRE_Int     logging;  /* extra computations for logging when logging>0 */
   HYPRE_Real  *norms;
   char    *log_file_name;

} hypre_GMRESData;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name generic GMRES Solver
 *
 * Description...
 **/
/*@{*/

/**
 * Description...
 *
 * @param param [IN] ...
 **/

hypre_GMRESFunctions *
hypre_GMRESFunctionsCreate(
   char *       (*CAlloc)        ( size_t count, size_t elt_size ),
   HYPRE_Int    (*Free)          ( char *ptr ),
   HYPRE_Int    (*CommInfo)      ( void  *A, HYPRE_Int   *my_id,
                                   HYPRE_Int   *num_procs ),
   void *       (*CreateVector)  ( void *vector ),
   void *       (*CreateVectorArray)  ( HYPRE_Int size, void *vectors ),
   HYPRE_Int    (*DestroyVector) ( void *vector ),
   void *       (*MatvecCreate)  ( void *A, void *x ),
   HYPRE_Int    (*Matvec)        ( void *matvec_data, HYPRE_Complex alpha, void *A,
                                   void *x, HYPRE_Complex beta, void *y ),
   HYPRE_Int    (*MatvecDestroy) ( void *matvec_data ),
   HYPRE_Real   (*InnerProd)     ( void *x, void *y ),
   HYPRE_Int    (*CopyVector)    ( void *x, void *y ),
   HYPRE_Int    (*ClearVector)   ( void *x ),
   HYPRE_Int    (*ScaleVector)   ( HYPRE_Complex alpha, void *x ),
   HYPRE_Int    (*Axpy)          ( HYPRE_Complex alpha, void *x, void *y ),
   HYPRE_Int    (*PrecondSetup)  ( void *vdata, void *A, void *b, void *x ),
   HYPRE_Int    (*Precond)       ( void *vdata, void *A, void *b, void *x )
   );

/**
 * Description...
 *
 * @param param [IN] ...
 **/

void *
hypre_GMRESCreate( hypre_GMRESFunctions *gmres_functions );

#ifdef __cplusplus
}
#endif
#endif

/******************************************************************************
 *
 * Preconditioned conjugate gradient (Omin) headers
 *
 *****************************************************************************/

#ifndef hypre_KRYLOV_PCG_HEADER
#define hypre_KRYLOV_PCG_HEADER

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

/**
 * @name Generic PCG Interface
 *
 * A general description of the interface goes here...
 *
 * @memo A generic PCG linear solver interface
 * @version 0.1
 * @author Jeffrey F. Painter
 **/
/*@{*/

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
 * hypre_PCGData and hypre_PCGFunctions
 *--------------------------------------------------------------------------*/

/**
 * @name PCG structs
 *
 * Description...
 **/
/*@{*/

/**
 * The {\tt hypre\_PCGSFunctions} object ...
 **/

typedef struct
{
   char *       (*CAlloc)        ( size_t count, size_t elt_size );
   HYPRE_Int    (*Free)          ( char *ptr );
   HYPRE_Int    (*CommInfo)      ( void  *A, HYPRE_Int   *my_id,
                                   HYPRE_Int   *num_procs );
   void *       (*CreateVector)  ( void *vector );
   HYPRE_Int    (*DestroyVector) ( void *vector );
   void *       (*MatvecCreate)  ( void *A, void *x );
   HYPRE_Int    (*Matvec)        ( void *matvec_data, HYPRE_Complex alpha, void *A,
                                   void *x, HYPRE_Complex beta, void *y );
   HYPRE_Int    (*MatvecDestroy) ( void *matvec_data );
   HYPRE_Real   (*InnerProd)     ( void *x, void *y );
   HYPRE_Int    (*CopyVector)    ( void *x, void *y );
   HYPRE_Int    (*ClearVector)   ( void *x );
   HYPRE_Int    (*ScaleVector)   ( HYPRE_Complex alpha, void *x );
   HYPRE_Int    (*Axpy)          ( HYPRE_Complex alpha, void *x, void *y );

	HYPRE_Int    (*precond)(void *vdata , void *A , void *b , void *x);
	HYPRE_Int    (*precond_setup)(void *vdata , void *A , void *b , void *x);

} hypre_PCGFunctions;

/**
 * The {\tt hypre\_PCGData} object ...
 **/

/*
 Summary of Parameters to Control Stopping Test:
 - Standard (default) error tolerance: |delta-residual|/|right-hand-side|<tol
 where the norm is an energy norm wrt preconditioner, |r|=sqrt(<Cr,r>).
 - two_norm!=0 means: the norm is the L2 norm, |r|=sqrt(<r,r>)
 - rel_change!=0 means: if pass the other stopping criteria, also check the
 relative change in the solution x.  Pass iff this relative change is small.
 - tol = relative error tolerance, as above
 -a_tol = absolute convergence tolerance (default is 0.0)
   If one desires the convergence test to check the absolute
   convergence tolerance *only*, then set the relative convergence
   tolerance to 0.0.  (The default convergence test is  <C*r,r> <=
   max(relative_tolerance^2 * <C*b, b>, absolute_tolerance^2)
- cf_tol = convergence factor tolerance; if >0 used for special test
  for slow convergence
- stop_crit!=0 means (TO BE PHASED OUT):
  pure absolute error tolerance rather than a pure relative
  error tolerance on the residual.  Never applies if rel_change!=0 or atolf!=0.
 - atolf = absolute error tolerance factor to be used _together_ with the
 relative error tolerance, |delta-residual| / ( atolf + |right-hand-side| ) < tol
  (To BE PHASED OUT)
 - recompute_residual means: when the iteration seems to be converged, recompute the
 residual from scratch (r=b-Ax) and use this new residual to repeat the convergence test.
 This can be expensive, use this only if you have seen a problem with the regular
 residual computation.
  - recompute_residual_p means: recompute the residual from scratch (r=b-Ax)
 every "recompute_residual_p" iterations.  This can be expensive and degrade the
 convergence. Use it only if you have seen a problem with the regular residual
 computation.
 */

typedef struct
{
   HYPRE_Real   tol;
   HYPRE_Real   atolf;
   HYPRE_Real   cf_tol;
   HYPRE_Real   a_tol;
   HYPRE_Real   rtol;
   HYPRE_Int      max_iter;
   HYPRE_Int      two_norm;
   HYPRE_Int      rel_change;
   HYPRE_Int      recompute_residual;
   HYPRE_Int      recompute_residual_p;
   HYPRE_Int      stop_crit;
   HYPRE_Int      converged;

   void    *A;
   void    *p;
   void    *s;
   void    *r; /* ...contains the residual.  This is currently kept permanently.
                  If that is ever changed, it still must be kept if logging>1 */

   HYPRE_Int      owns_matvec_data;  /* normally 1; if 0, don't delete it */
   void    *matvec_data;
   void    *precond_data;

   hypre_PCGFunctions * functions;

   /* log info (always logged) */
   HYPRE_Int      num_iterations;
   HYPRE_Real   rel_residual_norm;

   HYPRE_Int     print_level; /* printing when print_level>0 */
   HYPRE_Int     logging;  /* extra computations for logging when logging>0 */
   HYPRE_Real  *norms;
   HYPRE_Real  *rel_norms;

} hypre_PCGData;

#define hypre_PCGDataOwnsMatvecData(pcgdata)  ((pcgdata) -> owns_matvec_data)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name generic PCG Solver
 *
 * Description...
 **/
/*@{*/

/**
 * Description...
 *
 * @param param [IN] ...
 **/

hypre_PCGFunctions *
hypre_PCGFunctionsCreate(
   char *       (*CAlloc)        ( size_t count, size_t elt_size ),
   HYPRE_Int    (*Free)          ( char *ptr ),
   HYPRE_Int    (*CommInfo)      ( void  *A, HYPRE_Int   *my_id,
                                   HYPRE_Int   *num_procs ),
   void *       (*CreateVector)  ( void *vector ),
   HYPRE_Int    (*DestroyVector) ( void *vector ),
   void *       (*MatvecCreate)  ( void *A, void *x ),
   HYPRE_Int    (*Matvec)        ( void *matvec_data, HYPRE_Complex alpha, void *A,
                                   void *x, HYPRE_Complex beta, void *y ),
   HYPRE_Int    (*MatvecDestroy) ( void *matvec_data ),
   HYPRE_Real   (*InnerProd)     ( void *x, void *y ),
   HYPRE_Int    (*CopyVector)    ( void *x, void *y ),
   HYPRE_Int    (*ClearVector)   ( void *x ),
   HYPRE_Int    (*ScaleVector)   ( HYPRE_Complex alpha, void *x ),
   HYPRE_Int    (*Axpy)          ( HYPRE_Complex alpha, void *x, void *y ),
   HYPRE_Int    (*PrecondSetup)  ( void *vdata, void *A, void *b, void *x ),
   HYPRE_Int    (*Precond)       ( void *vdata, void *A, void *b, void *x )
   );

/**
 * Description...
 *
 * @param param [IN] ...
 **/

void *
hypre_PCGCreate( hypre_PCGFunctions *pcg_functions );

#ifdef __cplusplus
}
#endif

#endif

/* gmres.c */
void *hypre_GMRESCreate ( hypre_GMRESFunctions *gmres_functions );
HYPRE_Int hypre_GMRESDestroy ( void *gmres_vdata );
HYPRE_Int hypre_GMRESGetResidual ( void *gmres_vdata , void **residual );
HYPRE_Int hypre_GMRESSetup ( void *gmres_vdata , void *A , void *b , void *x );
HYPRE_Int hypre_GMRESSolve ( void *gmres_vdata , void *A , void *b , void *x );
HYPRE_Int hypre_GMRESSetKDim ( void *gmres_vdata , HYPRE_Int k_dim );
HYPRE_Int hypre_GMRESGetKDim ( void *gmres_vdata , HYPRE_Int *k_dim );
HYPRE_Int hypre_GMRESSetTol ( void *gmres_vdata , HYPRE_Real tol );
HYPRE_Int hypre_GMRESGetTol ( void *gmres_vdata , HYPRE_Real *tol );
HYPRE_Int hypre_GMRESSetAbsoluteTol ( void *gmres_vdata , HYPRE_Real a_tol );
HYPRE_Int hypre_GMRESGetAbsoluteTol ( void *gmres_vdata , HYPRE_Real *a_tol );
HYPRE_Int hypre_GMRESSetConvergenceFactorTol ( void *gmres_vdata , HYPRE_Real cf_tol );
HYPRE_Int hypre_GMRESGetConvergenceFactorTol ( void *gmres_vdata , HYPRE_Real *cf_tol );
HYPRE_Int hypre_GMRESSetMinIter ( void *gmres_vdata , HYPRE_Int min_iter );
HYPRE_Int hypre_GMRESGetMinIter ( void *gmres_vdata , HYPRE_Int *min_iter );
HYPRE_Int hypre_GMRESSetMaxIter ( void *gmres_vdata , HYPRE_Int max_iter );
HYPRE_Int hypre_GMRESGetMaxIter ( void *gmres_vdata , HYPRE_Int *max_iter );
HYPRE_Int hypre_GMRESSetRelChange ( void *gmres_vdata , HYPRE_Int rel_change );
HYPRE_Int hypre_GMRESGetRelChange ( void *gmres_vdata , HYPRE_Int *rel_change );
HYPRE_Int hypre_GMRESSetSkipRealResidualCheck ( void *gmres_vdata , HYPRE_Int skip_real_r_check );
HYPRE_Int hypre_GMRESGetSkipRealResidualCheck ( void *gmres_vdata , HYPRE_Int *skip_real_r_check );
HYPRE_Int hypre_GMRESSetStopCrit ( void *gmres_vdata , HYPRE_Int stop_crit );
HYPRE_Int hypre_GMRESGetStopCrit ( void *gmres_vdata , HYPRE_Int *stop_crit );
	HYPRE_Int hypre_GMRESSetPrecond ( void *gmres_vdata , HYPRE_Int (*precond )(void*,void*,void*,void*), HYPRE_Int (*precond_setup )(void*,void*,void*,void*), void *precond_data );
HYPRE_Int hypre_GMRESGetPrecond ( void *gmres_vdata , HYPRE_Solver *precond_data_ptr );
HYPRE_Int hypre_GMRESSetPrintLevel ( void *gmres_vdata , HYPRE_Int level );
HYPRE_Int hypre_GMRESGetPrintLevel ( void *gmres_vdata , HYPRE_Int *level );
HYPRE_Int hypre_GMRESSetLogging ( void *gmres_vdata , HYPRE_Int level );
HYPRE_Int hypre_GMRESGetLogging ( void *gmres_vdata , HYPRE_Int *level );
HYPRE_Int hypre_GMRESGetNumIterations ( void *gmres_vdata , HYPRE_Int *num_iterations );
HYPRE_Int hypre_GMRESGetConverged ( void *gmres_vdata , HYPRE_Int *converged );
HYPRE_Int hypre_GMRESGetFinalRelativeResidualNorm ( void *gmres_vdata , HYPRE_Real *relative_residual_norm );

/* HYPRE_gmres.c */
HYPRE_Int HYPRE_GMRESSetup ( HYPRE_Solver solver , HYPRE_Matrix A , HYPRE_Vector b , HYPRE_Vector x );
HYPRE_Int HYPRE_GMRESSolve ( HYPRE_Solver solver , HYPRE_Matrix A , HYPRE_Vector b , HYPRE_Vector x );
HYPRE_Int HYPRE_GMRESSetKDim ( HYPRE_Solver solver , HYPRE_Int k_dim );
HYPRE_Int HYPRE_GMRESGetKDim ( HYPRE_Solver solver , HYPRE_Int *k_dim );
HYPRE_Int HYPRE_GMRESSetTol ( HYPRE_Solver solver , HYPRE_Real tol );
HYPRE_Int HYPRE_GMRESGetTol ( HYPRE_Solver solver , HYPRE_Real *tol );
HYPRE_Int HYPRE_GMRESSetAbsoluteTol ( HYPRE_Solver solver , HYPRE_Real a_tol );
HYPRE_Int HYPRE_GMRESGetAbsoluteTol ( HYPRE_Solver solver , HYPRE_Real *a_tol );
HYPRE_Int HYPRE_GMRESSetConvergenceFactorTol ( HYPRE_Solver solver , HYPRE_Real cf_tol );
HYPRE_Int HYPRE_GMRESGetConvergenceFactorTol ( HYPRE_Solver solver , HYPRE_Real *cf_tol );
HYPRE_Int HYPRE_GMRESSetMinIter ( HYPRE_Solver solver , HYPRE_Int min_iter );
HYPRE_Int HYPRE_GMRESGetMinIter ( HYPRE_Solver solver , HYPRE_Int *min_iter );
HYPRE_Int HYPRE_GMRESSetMaxIter ( HYPRE_Solver solver , HYPRE_Int max_iter );
HYPRE_Int HYPRE_GMRESGetMaxIter ( HYPRE_Solver solver , HYPRE_Int *max_iter );
HYPRE_Int HYPRE_GMRESSetStopCrit ( HYPRE_Solver solver , HYPRE_Int stop_crit );
HYPRE_Int HYPRE_GMRESGetStopCrit ( HYPRE_Solver solver , HYPRE_Int *stop_crit );
HYPRE_Int HYPRE_GMRESSetRelChange ( HYPRE_Solver solver , HYPRE_Int rel_change );
HYPRE_Int HYPRE_GMRESGetRelChange ( HYPRE_Solver solver , HYPRE_Int *rel_change );
HYPRE_Int HYPRE_GMRESSetSkipRealResidualCheck ( HYPRE_Solver solver , HYPRE_Int skip_real_r_check );
HYPRE_Int HYPRE_GMRESGetSkipRealResidualCheck ( HYPRE_Solver solver , HYPRE_Int *skip_real_r_check );
HYPRE_Int HYPRE_GMRESSetPrecond ( HYPRE_Solver solver , HYPRE_PtrToSolverFcn precond , HYPRE_PtrToSolverFcn precond_setup , HYPRE_Solver precond_solver );
HYPRE_Int HYPRE_GMRESGetPrecond ( HYPRE_Solver solver , HYPRE_Solver *precond_data_ptr );
HYPRE_Int HYPRE_GMRESSetPrintLevel ( HYPRE_Solver solver , HYPRE_Int level );
HYPRE_Int HYPRE_GMRESGetPrintLevel ( HYPRE_Solver solver , HYPRE_Int *level );
HYPRE_Int HYPRE_GMRESSetLogging ( HYPRE_Solver solver , HYPRE_Int level );
HYPRE_Int HYPRE_GMRESGetLogging ( HYPRE_Solver solver , HYPRE_Int *level );
HYPRE_Int HYPRE_GMRESGetNumIterations ( HYPRE_Solver solver , HYPRE_Int *num_iterations );
HYPRE_Int HYPRE_GMRESGetConverged ( HYPRE_Solver solver , HYPRE_Int *converged );
HYPRE_Int HYPRE_GMRESGetFinalRelativeResidualNorm ( HYPRE_Solver solver , HYPRE_Real *norm );
HYPRE_Int HYPRE_GMRESGetResidual ( HYPRE_Solver solver , void **residual );

/* HYPRE_pcg.c */
HYPRE_Int HYPRE_PCGSetup ( HYPRE_Solver solver , HYPRE_Matrix A , HYPRE_Vector b , HYPRE_Vector x );
HYPRE_Int HYPRE_PCGSolve ( HYPRE_Solver solver , HYPRE_Matrix A , HYPRE_Vector b , HYPRE_Vector x );
HYPRE_Int HYPRE_PCGSetTol ( HYPRE_Solver solver , HYPRE_Real tol );
HYPRE_Int HYPRE_PCGGetTol ( HYPRE_Solver solver , HYPRE_Real *tol );
HYPRE_Int HYPRE_PCGSetAbsoluteTol ( HYPRE_Solver solver , HYPRE_Real a_tol );
HYPRE_Int HYPRE_PCGGetAbsoluteTol ( HYPRE_Solver solver , HYPRE_Real *a_tol );
HYPRE_Int HYPRE_PCGSetAbsoluteTolFactor ( HYPRE_Solver solver , HYPRE_Real abstolf );
HYPRE_Int HYPRE_PCGGetAbsoluteTolFactor ( HYPRE_Solver solver , HYPRE_Real *abstolf );
HYPRE_Int HYPRE_PCGSetResidualTol ( HYPRE_Solver solver , HYPRE_Real rtol );
HYPRE_Int HYPRE_PCGGetResidualTol ( HYPRE_Solver solver , HYPRE_Real *rtol );
HYPRE_Int HYPRE_PCGSetConvergenceFactorTol ( HYPRE_Solver solver , HYPRE_Real cf_tol );
HYPRE_Int HYPRE_PCGGetConvergenceFactorTol ( HYPRE_Solver solver , HYPRE_Real *cf_tol );
HYPRE_Int HYPRE_PCGSetMaxIter ( HYPRE_Solver solver , HYPRE_Int max_iter );
HYPRE_Int HYPRE_PCGGetMaxIter ( HYPRE_Solver solver , HYPRE_Int *max_iter );
HYPRE_Int HYPRE_PCGSetStopCrit ( HYPRE_Solver solver , HYPRE_Int stop_crit );
HYPRE_Int HYPRE_PCGGetStopCrit ( HYPRE_Solver solver , HYPRE_Int *stop_crit );
HYPRE_Int HYPRE_PCGSetTwoNorm ( HYPRE_Solver solver , HYPRE_Int two_norm );
HYPRE_Int HYPRE_PCGGetTwoNorm ( HYPRE_Solver solver , HYPRE_Int *two_norm );
HYPRE_Int HYPRE_PCGSetRelChange ( HYPRE_Solver solver , HYPRE_Int rel_change );
HYPRE_Int HYPRE_PCGGetRelChange ( HYPRE_Solver solver , HYPRE_Int *rel_change );
HYPRE_Int HYPRE_PCGSetRecomputeResidual ( HYPRE_Solver solver , HYPRE_Int recompute_residual );
HYPRE_Int HYPRE_PCGGetRecomputeResidual ( HYPRE_Solver solver , HYPRE_Int *recompute_residual );
HYPRE_Int HYPRE_PCGSetRecomputeResidualP ( HYPRE_Solver solver , HYPRE_Int recompute_residual_p );
HYPRE_Int HYPRE_PCGGetRecomputeResidualP ( HYPRE_Solver solver , HYPRE_Int *recompute_residual_p );
HYPRE_Int HYPRE_PCGSetPrecond ( HYPRE_Solver solver , HYPRE_PtrToSolverFcn precond , HYPRE_PtrToSolverFcn precond_setup , HYPRE_Solver precond_solver );
HYPRE_Int HYPRE_PCGGetPrecond ( HYPRE_Solver solver , HYPRE_Solver *precond_data_ptr );
HYPRE_Int HYPRE_PCGSetLogging ( HYPRE_Solver solver , HYPRE_Int level );
HYPRE_Int HYPRE_PCGGetLogging ( HYPRE_Solver solver , HYPRE_Int *level );
HYPRE_Int HYPRE_PCGSetPrintLevel ( HYPRE_Solver solver , HYPRE_Int level );
HYPRE_Int HYPRE_PCGGetPrintLevel ( HYPRE_Solver solver , HYPRE_Int *level );
HYPRE_Int HYPRE_PCGGetNumIterations ( HYPRE_Solver solver , HYPRE_Int *num_iterations );
HYPRE_Int HYPRE_PCGGetConverged ( HYPRE_Solver solver , HYPRE_Int *converged );
HYPRE_Int HYPRE_PCGGetFinalRelativeResidualNorm ( HYPRE_Solver solver , HYPRE_Real *norm );
HYPRE_Int HYPRE_PCGGetResidual ( HYPRE_Solver solver , void **residual );

/* pcg.c */
void *hypre_PCGCreate ( hypre_PCGFunctions *pcg_functions );
HYPRE_Int hypre_PCGDestroy ( void *pcg_vdata );
HYPRE_Int hypre_PCGGetResidual ( void *pcg_vdata , void **residual );
HYPRE_Int hypre_PCGSetup ( void *pcg_vdata , void *A , void *b , void *x );
HYPRE_Int hypre_PCGSolve ( void *pcg_vdata , void *A , void *b , void *x );
HYPRE_Int hypre_PCGSetTol ( void *pcg_vdata , HYPRE_Real tol );
HYPRE_Int hypre_PCGGetTol ( void *pcg_vdata , HYPRE_Real *tol );
HYPRE_Int hypre_PCGSetAbsoluteTol ( void *pcg_vdata , HYPRE_Real a_tol );
HYPRE_Int hypre_PCGGetAbsoluteTol ( void *pcg_vdata , HYPRE_Real *a_tol );
HYPRE_Int hypre_PCGSetAbsoluteTolFactor ( void *pcg_vdata , HYPRE_Real atolf );
HYPRE_Int hypre_PCGGetAbsoluteTolFactor ( void *pcg_vdata , HYPRE_Real *atolf );
HYPRE_Int hypre_PCGSetResidualTol ( void *pcg_vdata , HYPRE_Real rtol );
HYPRE_Int hypre_PCGGetResidualTol ( void *pcg_vdata , HYPRE_Real *rtol );
HYPRE_Int hypre_PCGSetConvergenceFactorTol ( void *pcg_vdata , HYPRE_Real cf_tol );
HYPRE_Int hypre_PCGGetConvergenceFactorTol ( void *pcg_vdata , HYPRE_Real *cf_tol );
HYPRE_Int hypre_PCGSetMaxIter ( void *pcg_vdata , HYPRE_Int max_iter );
HYPRE_Int hypre_PCGGetMaxIter ( void *pcg_vdata , HYPRE_Int *max_iter );
HYPRE_Int hypre_PCGSetTwoNorm ( void *pcg_vdata , HYPRE_Int two_norm );
HYPRE_Int hypre_PCGGetTwoNorm ( void *pcg_vdata , HYPRE_Int *two_norm );
HYPRE_Int hypre_PCGSetRelChange ( void *pcg_vdata , HYPRE_Int rel_change );
HYPRE_Int hypre_PCGGetRelChange ( void *pcg_vdata , HYPRE_Int *rel_change );
HYPRE_Int hypre_PCGSetRecomputeResidual ( void *pcg_vdata , HYPRE_Int recompute_residual );
HYPRE_Int hypre_PCGGetRecomputeResidual ( void *pcg_vdata , HYPRE_Int *recompute_residual );
HYPRE_Int hypre_PCGSetRecomputeResidualP ( void *pcg_vdata , HYPRE_Int recompute_residual_p );
HYPRE_Int hypre_PCGGetRecomputeResidualP ( void *pcg_vdata , HYPRE_Int *recompute_residual_p );
HYPRE_Int hypre_PCGSetStopCrit ( void *pcg_vdata , HYPRE_Int stop_crit );
HYPRE_Int hypre_PCGGetStopCrit ( void *pcg_vdata , HYPRE_Int *stop_crit );
HYPRE_Int hypre_PCGGetPrecond ( void *pcg_vdata , HYPRE_Solver *precond_data_ptr );
	HYPRE_Int hypre_PCGSetPrecond ( void *pcg_vdata , HYPRE_Int (*precond )(void*,void*,void*,void*), HYPRE_Int (*precond_setup )(void*,void*,void*,void*), void *precond_data );
HYPRE_Int hypre_PCGSetPrintLevel ( void *pcg_vdata , HYPRE_Int level );
HYPRE_Int hypre_PCGGetPrintLevel ( void *pcg_vdata , HYPRE_Int *level );
HYPRE_Int hypre_PCGSetLogging ( void *pcg_vdata , HYPRE_Int level );
HYPRE_Int hypre_PCGGetLogging ( void *pcg_vdata , HYPRE_Int *level );
HYPRE_Int hypre_PCGGetNumIterations ( void *pcg_vdata , HYPRE_Int *num_iterations );
HYPRE_Int hypre_PCGGetConverged ( void *pcg_vdata , HYPRE_Int *converged );
HYPRE_Int hypre_PCGPrintLogging ( void *pcg_vdata , HYPRE_Int myid );
HYPRE_Int hypre_PCGGetFinalRelativeResidualNorm ( void *pcg_vdata , HYPRE_Real *relative_residual_norm );

#ifdef __cplusplus
}
#endif

#endif

