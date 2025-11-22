CC := clang-fpchecker
CFLAGS := -std=c99 -Wall -Wextra -O2 -g -I../../../../src/
LDFLAGS :=

TARGETS := test_basic_functionality test_many_items
SRC := test_basic_functionality.c test_many_items.c
OBJS := $(SRC:.c=.o)

.PHONY: all clean run run_basic run_many

all: $(TARGETS)

%: %.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: test_basic_functionality
	./test_basic_functionality

run_basic: test_basic_functionality
	./test_basic_functionality

run_many: test_many_items
	./test_many_items

clean:
	rm -f $(OBJS) $(TARGETS)