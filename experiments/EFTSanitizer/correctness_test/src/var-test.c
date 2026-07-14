#include <stdio.h>
extern void eftsan_print_error(double);
double add(double x, double y){
  return x + y;
}
int main() {
  volatile double x, y, z1, z2, z3;
  z1 = 5;
  z2 = 6;
  z3 = z1 + z2;
  x = add(4, 5);
  y = add(6, z3);
  eftsan_print_error(x);
  eftsan_print_error(y);
}
