CC=gcc
CFLAGS=-O2 -w -I.
SRC=is.c c_timers.c c_print_results.c wtime.c
is_fp32: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -lm -o is_fp32
run: is_fp32
	./is_fp32
clean:
	rm -f is_fp32
