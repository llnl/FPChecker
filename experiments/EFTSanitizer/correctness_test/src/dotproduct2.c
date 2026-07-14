#include <stdio.h>
extern void eftsan_print_error(double);
// calculating dot product. The intermediate result goes close to the maximum dynamic range of posit, which will result in significant error in posit.
int main( int argc, const char* argv[] )
{
  float fa[4] = {3.200000000000000177635683940025E+0, 1.0E+0, -1.0E+0, 8.0E+0};
  float fb[4] = {4.0E+6, 1.0E+0, -1.0E+0, -1.6E+6};
  float fresult = 0.0;
  for (int i = 0; i < 4; i++) {
    fresult += fa[i] * fb[i];
  }
  eftsan_print_error(fresult);
  printf("Float result: %lf\n", fresult);
  
  return 0;
}
