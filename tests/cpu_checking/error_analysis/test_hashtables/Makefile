CC := clang-fpchecker
CFLAGS := -std=c99 -Wall -Wextra -O2 -g -I../../../../src/
LDFLAGS :=

TARGETS := test_basic_functionality test_many_items test_json
SRC := test_basic_functionality.c test_many_items.c test_json.c
OBJS := $(SRC:.c=.o)

.PHONY: all clean run run_basic run_many run_json

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

run_json: test_json
	./test_json

clean:
	rm -rf $(OBJS) $(TARGETS) .fpc_logs fpc-report .fpc_log.txt