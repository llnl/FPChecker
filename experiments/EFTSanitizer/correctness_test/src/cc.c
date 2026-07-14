#include <stdio.h>
#include <math.h> 

extern void eftsan_print_error(double);
int main(){
  float x = 1.0;
  float y = 0.9999999;
  float z = x - y;
  printf("z:%e", z);
  eftsan_print_error(z);
}
