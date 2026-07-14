#include <stdio.h>
extern void eftsan_print_error(double);
int main() {
  volatile double x = 0.0;
  while(x < 100.0) {
	x += 0.2;
  }
  eftsan_print_error(x);
  printf("%.20g\n", x);
}
