OP = -O2 -fno-vectorize -fno-slp-vectorize
CXX = FPC_INSTRUMENT_ERR_TRACKING=1 clang++-fpchecker

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
TARGET := main

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(OP)

%.o: %.cpp
	$(CXX) -c $< -o $@ $(OP)

clean:
	rm -rf $(OBJS) $(TARGET) __pycache__ .fpc_logs .fpc_log.txt fpc-report
