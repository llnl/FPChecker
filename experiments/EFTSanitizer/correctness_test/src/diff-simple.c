#include <stdio.h>
#include <math.h>
extern void eftsan_print_error(double);
int main() {
  double x,y;
  x = 1e16;
  y = (x + 1) - x;
  printf("%e\n", y);
  eftsan_print_error(y);
  return 0;
}
