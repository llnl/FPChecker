# NAS BT -- single tree, precision selected by PREC.
#   make PREC=-DNAS_FP32   make            make PREC=-DNAS_LD
CC      ?= clang
PREC    ?= 
OPT     ?= -O0
CFLAGS  ?= -g $(OPT) -std=c99 -I. $(PREC)
LDLIBS  ?= -lm
SRCS    := $(wildcard *.c)
TARGET  ?= bt

.PHONY: all clean
all: $(TARGET)
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDLIBS)
clean:
	rm -f $(TARGET) *.o
