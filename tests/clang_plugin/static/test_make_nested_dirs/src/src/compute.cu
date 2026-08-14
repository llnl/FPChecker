
#include <stdio.h>

__device__
double compute(double x)
{
  double y = x / (x + 1.3);
  return y;
}

