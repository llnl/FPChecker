
OP = 	-O2 -fno-vectorize -fno-slp-vectorize -DFPC_DEBUG_ERROR_ANALYSIS
CXX = FPC_INSTRUMENT_ERR_TRACKING=1 clang++-fpchecker 

all:
	$(CXX) -c multiple_comp.cpp $(OP)
	$(CXX) -o main multiple_comp.o

clean:
	rm -rf *.o main __pycache__ .fpc_logs .fpc_log.txt
