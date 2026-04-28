
OP = 	-O2 -fno-vectorize -fno-slp-vectorize #-DFPC_DEBUG_ERROR_ANALYSIS
CXX = FPC_INSTRUMENT_ERR_TRACKING=1 clang++-fpchecker 

all:
	$(CXX) -c memmove_test.cpp $(OP)
	$(CXX) -o memmove_test memmove_test.o

clean:
	rm -rf *.o memmove_test __pycache__ .fpc_logs .fpc_log.txt
