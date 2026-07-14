#include <stdio.h>
#include <math.h>
extern void eftsan_print_error(double);
double foo(double a) {
  return (a*sqrt(a))/(a+0.1);
}

double sum(double (*fn)(double)) {
  int i;
  double b = 0;

  for (i = 0; i < 100; ++i) {
    b += fn(b);
  }

  return b;
}

int main(int argc, char *argv[]) {
  double x = sum(foo);
  eftsan_print_error(x);
  printf("x:%e", x);
}
