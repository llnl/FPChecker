CC      ?= clang
PREC    ?= 
OPT     ?= -O0
CFLAGS  ?= -g $(OPT) -std=c99 -I. $(PREC)
LDLIBS  ?= -lm
TARGET  ?= cg
all: $(TARGET)
$(TARGET): $(wildcard *.c)
	$(CC) $(CFLAGS) $(wildcard *.c) -o $(TARGET) $(LDLIBS)
clean:
	rm -f $(TARGET) *.o
