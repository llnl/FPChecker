# CC = FPC_INSTRUMENT_ERR_TRACKING_FP64=1 OMPI_CC=clang-fpchecker mpicc

# CPPFLAGS = -I.
# CFLAGS = -O2 -fno-vectorize -fno-slp-vectorize
# LDFLAGS = -fopenmp
# LDLIBS = -lm

CC = FPC_INSTRUMENT_ERR_TRACKING_FP64=1 mpicc-fpchecker

MPI_LIB = $(shell mpicc -show | tr ' ' '\n' | grep -E '^-L|^-l|^-Wl,')

CPPFLAGS = -I.
CFLAGS   = -O2 -fopenmp -fno-vectorize -fno-slp-vectorize -ffp-contract=off
LDFLAGS  = -fopenmp
LDLIBS   = -lm $(MPI_LIB)

EXEC = miniAMR.x

OBJS = block.o check_sum.o comm_block.o comm_parent.o comm_refine.o \
       comm_util.o comm.o driver.o init.o main.o move.o pack.o plot.o \
       profile.o rcb.o refine.o sfc.o stencil.o util.o

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

all: $(EXEC)

run: $(EXEC)
	mpirun -np 1 ./$(EXEC) --npx 1 --npy 1 --npz 1 \
		--nx 8 --ny 8 --nz 8 --num_tsteps 20 --num_refine 2 --max_blocks 1000

clean:
	rm -f *.o miniAMR.x
	rm -rf .fpc_logs .fpc_log.txt fpc-report

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
