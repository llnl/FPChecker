
OP = 	-O2 -fno-vectorize -fno-slp-vectorize -DFPC_DEBUG_ERROR_ANALYSIS
CXX = FPC_INSTRUMENT_ERR_TRACKING=1 clang++-fpchecker 

add:
	$(CXX) -c basic_operations.cpp $(OP) -DADD_FUNCTION
	$(CXX) -o main basic_operations.o

sub:
	$(CXX) -c basic_operations.cpp $(OP) -DSUB_FUNCTION
	$(CXX) -o main basic_operations.o

mul:
	$(CXX) -c basic_operations.cpp $(OP) -DMUL_FUNCTION
	$(CXX) -o main basic_operations.o

div:
	$(CXX) -c basic_operations.cpp $(OP) -DDIV_FUNCTION
	$(CXX) -o main basic_operations.o

clean:
	rm -rf *.o main __pycache__ .fpc_logs .fpc_log.txt
