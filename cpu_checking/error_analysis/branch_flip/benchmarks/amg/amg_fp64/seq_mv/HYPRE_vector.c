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
 * HYPRE_Vector interface
 *
 *****************************************************************************/

#include "seq_mv.h"

/*--------------------------------------------------------------------------
 * HYPRE_VectorCreate
 *--------------------------------------------------------------------------*/

HYPRE_Vector
HYPRE_VectorCreate( HYPRE_Int size )
{
   return ( (HYPRE_Vector) hypre_SeqVectorCreate(size) );
}

/*--------------------------------------------------------------------------
 * HYPRE_VectorDestroy
 *--------------------------------------------------------------------------*/

HYPRE_Int 
HYPRE_VectorDestroy( HYPRE_Vector vector )
{
   return ( hypre_SeqVectorDestroy( (hypre_Vector *) vector ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_VectorInitialize
 *--------------------------------------------------------------------------*/

HYPRE_Int 
HYPRE_VectorInitialize( HYPRE_Vector vector )
{
   return ( hypre_SeqVectorInitialize( (hypre_Vector *) vector ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_VectorPrint
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_VectorPrint( HYPRE_Vector  vector,
                   char         *file_name )
{
   return ( hypre_SeqVectorPrint( (hypre_Vector *) vector,
                      file_name ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_VectorRead
 *--------------------------------------------------------------------------*/

HYPRE_Vector
HYPRE_VectorRead( char         *file_name )
{
   return ( (HYPRE_Vector) hypre_SeqVectorRead( file_name ) );
}
