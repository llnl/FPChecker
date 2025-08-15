
OP = 	-O2
CXX = FPC_INSTRUMENT=1 clang++-fpchecker 

all:
	$(CXX) -c simple_sum.cpp $(OP)
	$(CXX) -o main simple_sum.o

clean:
	rm -rf *.o main __pycache__ .fpc_logs .fpc_log.txt
