
OP = 	-O2 #-DFPC_DEBUG_ERROR_ANALYSIS
CC = clang-fpchecker 

all:
	$(CC) -c test.c $(OP)
	$(CC) -o test test.o

clean:
	rm -rf *.o test __pycache__ .fpc_logs .fpc_log.txt