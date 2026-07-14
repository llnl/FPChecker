#include <stdio.h>
extern void eftsan_print_error(double);
int main() {
  int x;
  x = 1 + 2;
  printf("%d\n", x);
  eftsan_print_error(x);
  return 0;
}
