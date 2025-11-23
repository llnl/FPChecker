OP = 	-O2 #-DFPC_DEBUG_ERROR_ANALYSIS
CXX = FPC_INSTRUMENT_ERR_TRACKING=1 clang++-fpchecker

SRCS = simple_sum.cpp test_math_calls.cpp
OBJS = $(SRCS:.cpp=.o)
TARGETS = simple_sum test_math_calls

all: $(TARGETS)

simple_sum: simple_sum.o
	$(CXX) -o $@ $^

test_math_calls: test_math_calls.o
	$(CXX) -o $@ $^

%.o: %.cpp
	$(CXX) -c $< $(OP)

clean:
	rm -rf *.o $(TARGETS) main __pycache__ .fpc_logs .fpc_log.txt
