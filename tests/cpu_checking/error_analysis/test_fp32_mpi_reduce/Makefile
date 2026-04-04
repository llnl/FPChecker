
OP = 	-O2 -fno-vectorize -fno-slp-vectorize
CXX = FPC_INSTRUMENT_ERR_TRACKING=1 mpicxx-fpchecker

all:
	$(CXX) -c reduce_test.cpp $(OP)
	$(CXX) -o reduce_test reduce_test.o

clean:
	rm -rf *.o reduce_test __pycache__ .fpc_logs .fpc_log.txt
