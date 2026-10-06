CC   = mpicc
LD   = mpicc
CPPFLAGS = -I.
FPCHECKER_ROOT ?= ../../../..
FPCHECKER_BUILD_DIR ?= $(FPCHECKER_ROOT)/build
FPCHECKER_RUNTIME ?= $(FPCHECKER_ROOT)/src/Runtime_error_FP64.h
FPCHECKER_PLUGIN ?= $(FPCHECKER_BUILD_DIR)/libfpchecker_error_fp64.so
FPCHECKER_FLAGS ?= -g -include $(FPCHECKER_RUNTIME) -fpass-plugin=$(FPCHECKER_PLUGIN)
CFLAGS = -O2 -fno-vectorize -fno-slp-vectorize -ffp-contract=off $(FPCHECKER_FLAGS)
LDFLAGS =
LDLIBS = -lm

EXEC = miniAMR.x

OBJS = block.o check_sum.o comm_block.o comm.o comm_parent.o comm_refine.o \
       comm_util.o driver.o init.o main.o move.o pack.o plot.o profile.o \
       rcb.o refine.o sfc.o stencil.o util.o

$(EXEC): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

clean:
	rm -f *.o miniAMR.x
	rm -rf .fpc_logs ../.fpc_logs

# need dependencies

block.o: block.h proto.h comm.h timer.h

check_sum.o: block.h comm.h proto.h timer.h

comm_block.o: block.h comm.h proto.h

comm.o: block.h comm.h proto.h timer.h

comm_parent.o: block.h comm.h proto.h

comm_refine.o: block.h comm.h proto.h

comm_util.o: block.h comm.h proto.h

driver.o: block.h comm.h proto.h timer.h

init.o: block.h proto.h comm.h

main.o: block.h comm.h proto.h param.h timer.h

move.o: block.h comm.h proto.h

pack.o: block.h comm.h proto.h

plot.o: block.h comm.h proto.h

profile.o: block.h comm.h proto.h timer.h

rcb.o: block.h comm.h proto.h timer.h

refine.o: block.h comm.h proto.h timer.h

sfc.o: block.h comm.h proto.h timer.h

stencil.o: block.h comm.h proto.h

util.o: block.h comm.h proto.h timer.h
