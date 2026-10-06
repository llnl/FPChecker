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


CFLAGS = \
 -I..\
 -I../utilities\
 -I../seq_mv\
 ${INCLUDE_CFLAGS}

RANLIB = ranlib

HEADERS =\
 headers.h\
 HYPRE_parcsr_mv.h\
 _hypre_parcsr_mv.h\
 new_commpkg.h\
 par_csr_communication.h\
 par_csr_matrix.h\
 par_vector.h

FILES =\
 HYPRE_parcsr_matrix.c\
 HYPRE_parcsr_vector.c\
 new_commpkg.c\
 par_csr_assumed_part.c\
 par_csr_communication.c\
 par_csr_matop.c\
 par_csr_matrix.c\
 par_csr_matvec.c\
 par_vector.c
# par_csr_matop_marked.c\

OBJS = ${FILES:.c=.o}

##################################################################
# Targets
##################################################################

all: libparcsr_mv.a

clean:
	rm -f *.o *.dxx

veryclean: clean
	rm -f lib*.a

##################################################################
# Rules
##################################################################

libparcsr_mv.a: ${OBJS}
	@echo  "Building $@ ... "
	ar -rcu $@ ${OBJS}
	${RANLIB} $@

${OBJS}: ${HEADERS}

##################################################################
# Generic rules
##################################################################

c.o:
	${CC} -o $@ -c ${CFLAGS} $<
