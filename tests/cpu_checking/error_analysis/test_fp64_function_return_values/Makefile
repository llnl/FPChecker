OP = 	-O2 -fno-vectorize -fno-slp-vectorize
CXX = FPC_INSTRUMENT_ERR_TRACKING_FP64=1 clang++-fpchecker

all:
	$(CXX) -c main.cpp $(OP)
	$(CXX) -o main main.o $(OP)

clean:
	rm -rf *.o main __pycache__ .fpc_logs .fpc_log.txt fpc-report
