OP = -O2 -fno-vectorize -fno-slp-vectorize #-DFPC_DEBUG_ERROR_ANALYSIS
CXX = FPC_INSTRUMENT_ERR_TRACKING_FP64=1 clang++-fpchecker

all:
	$(CXX) -c memcpy_fp64_test.cpp $(OP)
	$(CXX) -o memcpy_test memcpy_fp64_test.o

clean:
	rm -rf *.o memcpy_test __pycache__ .fpc_logs .fpc_log.txt