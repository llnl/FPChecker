
OP = 	-O2 -fno-vectorize -fno-slp-vectorize -Wno-format -std=c++17 -DFPC_DEBUG_ERROR_ANALYSIS
CXX = FPC_INSTRUMENT=1 clang++-fpchecker 

all:
	$(CXX) -c solver.cpp $(OP)
	$(CXX) -o main solver.o $(OP)

clean:
	rm -rf *.o main __pycache__ .fpc_logs .fpc_log.txt
