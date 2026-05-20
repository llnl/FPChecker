
OP = 	-O2 -fno-vectorize -fno-slp-vectorize #-DFPC_DEBUG_ERROR_ANALYSIS #-DFPDC_DEBUG_CALLSTACK
CXX = FPC_INSTRUMENT_ERR_TRACKING=1 clang++-fpchecker 
#CXX = clang++-fpchecker 

all:
	$(CXX) -c vector_dot_product.cpp $(OP)
	$(CXX) -o main vector_dot_product.o

clean:
	rm -rf *.o main __pycache__ .fpc_logs .fpc_log.txt fpc-report
