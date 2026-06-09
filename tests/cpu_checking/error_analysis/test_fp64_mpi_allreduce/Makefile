
OP = 	-O2 -fno-vectorize -fno-slp-vectorize
CXX = FPC_INSTRUMENT_ERR_TRACKING_FP64=1 mpicxx-fpchecker

all:
	$(CXX) -c allreduce_test.cpp $(OP)
	$(CXX) -o allreduce_test allreduce_test.o

clean:
	rm -rf *.o allreduce_test __pycache__ .fpc_logs .fpc_log.txt
