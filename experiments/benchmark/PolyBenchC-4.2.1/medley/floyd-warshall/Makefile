OP = -O2 -fno-vectorize -fno-slp-vectorize

CC = FPC_INSTRUMENT_ERR_TRACKING=1 clang-fpchecker

INCLUDE_PATH = -I. -I../../utilities
DATASET = -DMINI_DATASET
DUMP = -DPOLYBENCH_DUMP_ARRAYS

all:
	$(CC) -c floyd-warshall.c $(OP) $(INCLUDE_PATH) $(DATASET) $(DUMP)
	$(CC) -c ../../utilities/polybench.c $(OP) $(INCLUDE_PATH) $(DATASET) $(DUMP)
	$(CC) -o floyd-warshall floyd-warshall.o polybench.o -lm

clean:
	rm -rf *.o floyd-warshall __pycache__ .fpc_logs .fpc_log.txt
