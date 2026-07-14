#include <stdio.h>
extern void eftsan_print_error(double);
double addTwo(double x, double y){
  return x + y;
}

int main() {
  volatile double x, y, z;
  z = 3;
  x = addTwo(z + 2, 4);
  y = addTwo(5, 7);
  eftsan_print_error(x);
  eftsan_print_error(y);
  printf("%e\n%e\n", x, y);
}
