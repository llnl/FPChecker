
OP = 	-O2 -fno-vectorize -fno-slp-vectorize
CXX = FPC_INSTRUMENT_ERR_TRACKING=1 mpicxx-fpchecker

all:
	$(CXX) -c bcast_test.cpp $(OP)
	$(CXX) -o bcast_test bcast_test.o

clean:
	rm -rf *.o bcast_test __pycache__ .fpc_logs .fpc_log.txt
