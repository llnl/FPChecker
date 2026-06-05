OP = -O2 -fno-vectorize -fno-slp-vectorize #-DFPC_DEBUG_ERROR_ANALYSIS
CXX = FPC_INSTRUMENT_ERR_TRACKING_FP64=1 clang++-fpchecker

all:
	$(CXX) -c simple_sum.cpp $(OP)
	$(CXX) -o simple_sum simple_sum.o

clean:
	rm -rf *.o simple_sum __pycache__ .fpc_logs .fpc_log.txt
