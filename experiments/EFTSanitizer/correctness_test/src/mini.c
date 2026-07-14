#include <stdio.h>
#include <math.h>
extern void eftsan_print_error(double);
int main() {
  volatile double x,y;
  x = 3;
  y = sqrt(x + 1);
  printf("%e\n", y);
  eftsan_print_error(y);
  return 0;
}
