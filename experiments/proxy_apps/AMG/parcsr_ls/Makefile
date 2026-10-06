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
 -I../krylov\
 -I../seq_mv\
 -I../parcsr_mv\
 -I../IJ_mv\
 ${INCLUDE_CFLAGS}

HEADERS =\
 ams.h\
 HYPRE_parcsr_ls.h\
 _hypre_parcsr_ls.h\
 aux_interp.h\
 headers.h\
 par_amg.h

FILES =\
 ams.c\
 aux_interp.c\
 gen_redcs_mat.c\
 HYPRE_parcsr_amg.c\
 HYPRE_parcsr_gmres.c\
 HYPRE_parcsr_pcg.c\
 par_amg.c\
 par_amg_setup.c\
 par_amg_solve.c\
 par_cg_relax_wt.c\
 par_coarsen.c\
 par_cheby.c\
 par_coarse_parms.c\
 par_cycle.c\
 par_add_cycle.c\
 par_difconv.c\
 par_indepset.c\
 par_interp.c\
 par_multi_interp.c\
 par_lr_interp.c\
 par_nongalerkin.c\
 par_rap.c\
 par_rap_communication.c\
 par_relax.c\
 par_relax_more.c\
 par_relax_interface.c\
 par_scaled_matnorm.c\
 par_stats.c\
 par_strength.c\
 partial.c\
 pcg_par.c

OBJS = ${FILES:.c=.o}

##################################################################
# Targets
##################################################################

all: libparcsr_ls.a

clean:
	@rm -f *.o *.dxx

veryclean: clean
	@rm -f lib*.a

##################################################################
# Rules
##################################################################

libparcsr_ls.a: ${OBJS}
	@echo  "Building $@ ... "
	ar -rcu $@ ${OBJS}
	ranlib $@

${OBJS}: ${HEADERS}

##################################################################
# Generic rules
##################################################################

.c.o:
	${CC} -o $@ -c ${CFLAGS} $<
