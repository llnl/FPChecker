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
#ifndef hypre_MV_HEADER
#define hypre_MV_HEADER

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "HYPRE_seq_mv.h"

#include "../utilities/_hypre_utilities.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
 *
 * Header info for CSR Matrix data structures
 *
 * Note: this matrix currently uses 0-based indexing.
 *
 *****************************************************************************/

#ifndef hypre_CSR_MATRIX_HEADER
#define hypre_CSR_MATRIX_HEADER

/*--------------------------------------------------------------------------
 * CSR Matrix
 *--------------------------------------------------------------------------*/

typedef struct
{
   HYPRE_Int     *i;
   HYPRE_Int     *j;
   HYPRE_Int      num_rows;
   HYPRE_Int      num_cols;
   HYPRE_Int      num_nonzeros;

   /* Does the CSRMatrix create/destroy `data', `i', `j'? */
   HYPRE_Int      owns_data;

   HYPRE_Complex  *data;

   /* for compressing rows in matrix multiplication  */
   HYPRE_Int     *rownnz;
   HYPRE_Int      num_rownnz;

} hypre_CSRMatrix;

/*--------------------------------------------------------------------------
 * Accessor functions for the CSR Matrix structure
 *--------------------------------------------------------------------------*/

#define hypre_CSRMatrixData(matrix)         ((matrix) -> data)
#define hypre_CSRMatrixI(matrix)            ((matrix) -> i)
#define hypre_CSRMatrixJ(matrix)            ((matrix) -> j)
#define hypre_CSRMatrixNumRows(matrix)      ((matrix) -> num_rows)
#define hypre_CSRMatrixNumCols(matrix)      ((matrix) -> num_cols)
#define hypre_CSRMatrixNumNonzeros(matrix)  ((matrix) -> num_nonzeros)
#define hypre_CSRMatrixRownnz(matrix)       ((matrix) -> rownnz)
#define hypre_CSRMatrixNumRownnz(matrix)    ((matrix) -> num_rownnz)
#define hypre_CSRMatrixOwnsData(matrix)     ((matrix) -> owns_data)

HYPRE_Int hypre_CSRMatrixGetLoadBalancedPartitionBegin( hypre_CSRMatrix *A );
HYPRE_Int hypre_CSRMatrixGetLoadBalancedPartitionEnd( hypre_CSRMatrix *A );

#endif


/******************************************************************************
 *
 * Header info for Vector data structure
 *
 *****************************************************************************/

#ifndef hypre_VECTOR_HEADER
#define hypre_VECTOR_HEADER

/*--------------------------------------------------------------------------
 * hypre_Vector
 *--------------------------------------------------------------------------*/

typedef struct
{
   HYPRE_Complex  *data;
   HYPRE_Int      size;

   /* Does the Vector create/destroy `data'? */
   HYPRE_Int      owns_data;

   /* For multivectors...*/
   HYPRE_Int   num_vectors;  /* the above "size" is size of one vector */
   HYPRE_Int   multivec_storage_method;
   /* ...if 0, store colwise v0[0], v0[1], ..., v1[0], v1[1], ... v2[0]... */
   /* ...if 1, store rowwise v0[0], v1[0], ..., v0[1], v1[1], ... */
   /* With colwise storage, vj[i] = data[ j*size + i]
      With rowwise storage, vj[i] = data[ j + num_vectors*i] */
   HYPRE_Int  vecstride, idxstride;
   /* ... so vj[i] = data[ j*vecstride + i*idxstride ] regardless of row_storage.*/

} hypre_Vector;

/*--------------------------------------------------------------------------
 * Accessor functions for the Vector structure
 *--------------------------------------------------------------------------*/

#define hypre_VectorData(vector)      ((vector) -> data)
#define hypre_VectorSize(vector)      ((vector) -> size)
#define hypre_VectorOwnsData(vector)  ((vector) -> owns_data)
#define hypre_VectorNumVectors(vector) ((vector) -> num_vectors)
#define hypre_VectorMultiVecStorageMethod(vector) ((vector) -> multivec_storage_method)
#define hypre_VectorVectorStride(vector) ((vector) -> vecstride )
#define hypre_VectorIndexStride(vector) ((vector) -> idxstride )

#endif


/* csr_matop.c */
hypre_CSRMatrix *hypre_CSRMatrixAdd ( hypre_CSRMatrix *A , hypre_CSRMatrix *B );
hypre_CSRMatrix *hypre_CSRMatrixMultiply ( hypre_CSRMatrix *A , hypre_CSRMatrix *B );
hypre_CSRMatrix *hypre_CSRMatrixDeleteZeros ( hypre_CSRMatrix *A , HYPRE_Real tol );
HYPRE_Int hypre_CSRMatrixTranspose ( hypre_CSRMatrix *A , hypre_CSRMatrix **AT , HYPRE_Int data );
HYPRE_Int hypre_CSRMatrixReorder ( hypre_CSRMatrix *A );
HYPRE_Complex hypre_CSRMatrixSumElts ( hypre_CSRMatrix *A );

/* csr_matrix.c */
hypre_CSRMatrix *hypre_CSRMatrixCreate ( HYPRE_Int num_rows , HYPRE_Int num_cols , HYPRE_Int num_nonzeros );
HYPRE_Int hypre_CSRMatrixDestroy ( hypre_CSRMatrix *matrix );
HYPRE_Int hypre_CSRMatrixInitialize ( hypre_CSRMatrix *matrix );
HYPRE_Int hypre_CSRMatrixSetDataOwner ( hypre_CSRMatrix *matrix , HYPRE_Int owns_data );
HYPRE_Int hypre_CSRMatrixSetRownnz ( hypre_CSRMatrix *matrix );
hypre_CSRMatrix *hypre_CSRMatrixRead ( char *file_name );
HYPRE_Int hypre_CSRMatrixPrint ( hypre_CSRMatrix *matrix , char *file_name );
HYPRE_Int hypre_CSRMatrixPrintHB ( hypre_CSRMatrix *matrix_input , char *file_name );
HYPRE_Int hypre_CSRMatrixCopy ( hypre_CSRMatrix *A , hypre_CSRMatrix *B , HYPRE_Int copy_data );
hypre_CSRMatrix *hypre_CSRMatrixClone ( hypre_CSRMatrix *A );
hypre_CSRMatrix *hypre_CSRMatrixUnion ( hypre_CSRMatrix *A , hypre_CSRMatrix *B , HYPRE_Int *col_map_offd_A , HYPRE_Int *col_map_offd_B , HYPRE_Int **col_map_offd_C );

/* csr_matvec.c */
// y[offset:end] = alpha*A[offset:end,:]*x + beta*b[offset:end]
HYPRE_Int hypre_CSRMatrixMatvecOutOfPlace ( HYPRE_Complex alpha , hypre_CSRMatrix *A , hypre_Vector *x , HYPRE_Complex beta , hypre_Vector *b, hypre_Vector *y, HYPRE_Int offset );
// y = alpha*A + beta*y
HYPRE_Int hypre_CSRMatrixMatvec ( HYPRE_Complex alpha , hypre_CSRMatrix *A , hypre_Vector *x , HYPRE_Complex beta , hypre_Vector *y );
HYPRE_Int hypre_CSRMatrixMatvecT ( HYPRE_Complex alpha , hypre_CSRMatrix *A , hypre_Vector *x , HYPRE_Complex beta , hypre_Vector *y );
HYPRE_Int hypre_CSRMatrixMatvec_FF ( HYPRE_Complex alpha , hypre_CSRMatrix *A , hypre_Vector *x , HYPRE_Complex beta , hypre_Vector *y , HYPRE_Int *CF_marker_x , HYPRE_Int *CF_marker_y , HYPRE_Int fpt );

/* genpart.c */
HYPRE_Int hypre_GeneratePartitioning ( HYPRE_Int length , HYPRE_Int num_procs , HYPRE_Int **part_ptr );
HYPRE_Int hypre_GenerateLocalPartitioning ( HYPRE_Int length , HYPRE_Int num_procs , HYPRE_Int myid , HYPRE_Int **part_ptr );

/* HYPRE_csr_matrix.c */
HYPRE_CSRMatrix HYPRE_CSRMatrixCreate ( HYPRE_Int num_rows , HYPRE_Int num_cols , HYPRE_Int *row_sizes );
HYPRE_Int HYPRE_CSRMatrixDestroy ( HYPRE_CSRMatrix matrix );
HYPRE_Int HYPRE_CSRMatrixInitialize ( HYPRE_CSRMatrix matrix );
HYPRE_CSRMatrix HYPRE_CSRMatrixRead ( char *file_name );
void HYPRE_CSRMatrixPrint ( HYPRE_CSRMatrix matrix , char *file_name );
HYPRE_Int HYPRE_CSRMatrixGetNumRows ( HYPRE_CSRMatrix matrix , HYPRE_Int *num_rows );

/* vector.c */
hypre_Vector *hypre_SeqVectorCreate ( HYPRE_Int size );
hypre_Vector *hypre_SeqMultiVectorCreate ( HYPRE_Int size , HYPRE_Int num_vectors );
HYPRE_Int hypre_SeqVectorDestroy ( hypre_Vector *vector );
HYPRE_Int hypre_SeqVectorInitialize ( hypre_Vector *vector );
HYPRE_Int hypre_SeqVectorSetDataOwner ( hypre_Vector *vector , HYPRE_Int owns_data );
hypre_Vector *hypre_SeqVectorRead ( char *file_name );
HYPRE_Int hypre_SeqVectorPrint ( hypre_Vector *vector , char *file_name );
HYPRE_Int hypre_SeqVectorSetConstantValues ( hypre_Vector *v , HYPRE_Complex value );
HYPRE_Int hypre_SeqVectorSetRandomValues ( hypre_Vector *v , HYPRE_Int seed );
HYPRE_Int hypre_SeqVectorCopy ( hypre_Vector *x , hypre_Vector *y );
hypre_Vector *hypre_SeqVectorCloneDeep ( hypre_Vector *x );
hypre_Vector *hypre_SeqVectorCloneShallow ( hypre_Vector *x );
HYPRE_Int hypre_SeqVectorScale ( HYPRE_Complex alpha , hypre_Vector *y );
HYPRE_Int hypre_SeqVectorAxpy ( HYPRE_Complex alpha , hypre_Vector *x , hypre_Vector *y );
HYPRE_Real hypre_SeqVectorInnerProd ( hypre_Vector *x , hypre_Vector *y );
HYPRE_Complex hypre_VectorSumElts ( hypre_Vector *vector );
#ifdef __cplusplus
}
#endif

#endif

