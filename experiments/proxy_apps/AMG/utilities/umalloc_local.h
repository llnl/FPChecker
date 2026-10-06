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



#ifndef UMALLOC_LOCAL_HEADER
#define UMALLOC_LOCAL_HEADER

#ifdef HYPRE_USE_UMALLOC
#include <umalloc.h>

#define MAX_THREAD_COUNT 10 
#define INITIAL_HEAP_SIZE 500000
#define GET_MISS_COUNTS

struct upc_struct
{
	Heap_t myheap;
};

void *_uinitial_block[MAX_THREAD_COUNT];
struct upc_struct _uparam[MAX_THREAD_COUNT];

HYPRE_Int _uheapReleasesCount=0;
HYPRE_Int _uheapGetsCount=0;

void *_uget_fn(Heap_t usrheap, size_t *length, HYPRE_Int *clean);
void _urelease_fn(Heap_t usrheap, void *p, size_t size);

#endif

#endif
