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
 -I../utilities\
 ${INCLUDE_CFLAGS}

RANLIB = ranlib

HEADERS =\
 csr_matrix.h\
 headers.h\
 HYPRE_seq_mv.h\
 seq_mv.h\
 vector.h

FILES =\
 csr_matop.c\
 csr_matrix.c\
 csr_matvec.c\
 genpart.c\
 HYPRE_csr_matrix.c\
 HYPRE_vector.c\
 vector.c

OBJS = ${FILES:.c=.o}

##################################################################
# Targets
##################################################################

all: libseq_mv.a

clean:
	@rm -f *.o *.dxx

veryclean: clean
	@rm -f lib*.a

##################################################################
# Rules
##################################################################

libseq_mv.a: ${OBJS} ${CUOBJS}
	@echo  "Building $@ ... "
	ar -rcu $@ ${OBJS}
	${RANLIB} $@

${OBJS}: ${HEADERS}

##################################################################
# Generic rules
##################################################################

.c.o:
	${CC} -o $@ -c ${CFLAGS} $<

