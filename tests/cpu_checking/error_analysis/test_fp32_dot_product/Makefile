
OP = 	-O2
CXX = FPC_INSTRUMENT=1 clang++-fpchecker 
#CXX = clang++-fpchecker 

all:
	$(CXX) -c vector_dot_product.cpp $(OP)
	$(CXX) -o main vector_dot_product.o

clean:
	rm -rf *.o main __pycache__ .fpc_logs .fpc_log.txt
