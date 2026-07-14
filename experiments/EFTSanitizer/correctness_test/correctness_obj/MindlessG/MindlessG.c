#include <stdio.h>
#include <math.h>
extern void eftsan_print_error(double);
// William Kahan's Mindless G example.
double T(double z)
{
    double k, diff;
    if (z == 0.0) {
        k = 1.0f;
    } else {
	diff = exp(z) - 1.0f;
        k = (diff) / z;
    }
    return k;
}

double Q(double y)
{
  double z1 = y * y + 1.0f;
  double z2 = y - sqrt(z1); 
  double t1 = fabs(z2);
  double t2 = 1.0f / (y + sqrt(z1));
  return t1 - t2;
}

double G(double x)
{
    return T(Q(x) * Q(x));
}

int main( int argc, const char* argv[] )
{
    printf("Starting Mindless_G testing. The result should be G(n) = 1 for all n\n");
    for (int i = 0; i < 9999; i++) {
	    double res = G(i);
      eftsan_print_error(res);
        printf("G(%d) = %.10e\n", i, res);
    }
    return 0;
}
