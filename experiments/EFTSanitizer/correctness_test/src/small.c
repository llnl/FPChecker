#include <stdio.h>
#include <math.h>
extern void eftsan_print_error(double);
void calcY(double* y, double x){
  *y = sqrt(x + 1) - sqrt(x);
}

int main() {
  double x,y;
  x = 1e10;
  calcY(&y, x);
  calcY(&y, x);
  printf("%e\n", y);
  eftsan_print_error(y);
  return 0;
}
