OP = -O2 -fno-vectorize -fno-slp-vectorize -std=c++17
CXX = FPC_INSTRUMENT_ERR_TRACKING=1 clang++-fpchecker

SRCS = main.cpp transform.cpp consume.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean

all: main

main: $(OBJS)
	$(CXX) $(OBJS) -o $@ $(OP)

%.o: %.cpp api.h
	$(CXX) -c $< -o $@ $(OP)

clean:
	rm -rf $(OBJS) main __pycache__ .fpc_logs .fpc_log.txt fpc-report
