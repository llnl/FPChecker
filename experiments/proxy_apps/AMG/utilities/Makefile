#BHEADER**********************************************************************
# Copyright (c) 2017,  Lawrence Livermore National Security, LLC.
# Produced at the Lawrence Livermore National Laboratory.
# Written by Ulrike Yang (yang11@llnl.gov) et al. CODE-LLNL-738-322.
# This file is part of AMG.  See files COPYRIGHT and README for details.
#
# AMG is free software; you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License (as published by the Free
# Software Foundation) version 2.1 dated February 1999.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the terms and conditions of the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#
#EHEADER**********************************************************************


.SUFFIXES:
.SUFFIXES: .c .f .o

include ../Makefile.include

CFLAGS =\
   -I..\
   ${INCLUDE_CFLAGS}\
   ${CDEFS}
CXX_COMPILE_FLAGS = ${CFLAGS}

RANLIB = ranlib

##################################################################
# Targets
##################################################################

all: libHYPRE_utilities.a

clean:
	@rm -f *.o 

veryclean: clean
	@rm -f libHYPRE_*.a

##################################################################
# Utilities rules
##################################################################

HEADERS =\
 amg_linklist.h\
 exchange_data.h\
 general.h\
 hypre_memory.h\
 HYPRE_utilities.h\
 mpistubs.h\
 threading.h\
 timing.h\
 umalloc_local.h\
 _hypre_utilities.h

FILES =\
 amg_linklist.c\
 binsearch.c\
 exchange_data.c\
 fortran_matrix.c\
 hypre_complex.c\
 hypre_memory.c\
 hypre_printf.c\
 hypre_qsort.c\
 memory_dmalloc.c\
 mpistubs.c\
 qsplit.c\
 random.c\
 threading.c\
 timer.c\
 timing.c\
 umalloc_local.c\
 hypre_error.c\
 F90_HYPRE_error.c\
 hypre_prefix_sum.c\
 hypre_merge_sort.c\
 hypre_hopscotch_hash.c

OBJS = ${FILES:.c=.o}

##################################################################
# Rules
##################################################################

libHYPRE_utilities.a: ${OBJS}
	@echo  "Building $@ ... "
	ar -rcu $@ *.o
	${RANLIB} $@

${OBJS}: ${HEADERS}

timer.o: timer.c
	${CC} -D_POSIX_SOURCE -o $@ -c ${CFLAGS} $<

##################################################################
# Generic rules
##################################################################

.c.o:
	${CC} -o $@ -c ${CFLAGS} $<
