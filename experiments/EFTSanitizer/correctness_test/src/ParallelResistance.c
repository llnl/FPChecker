#include <stdio.h>
#include <math.h>
extern void eftsan_print_error(double);
// Let's calculate the parallel resistance function.
// The parallel resistance of two resistors is 1 / (1/r1 + 1/r2)
// Benchmark inspired by https://randomascii.wordpress.com/2012/04/21/exceptional-floating-point/
int main(int argc, const char* argv[] )
{
    float r1 = 0;
    float r2 = 0;
    printf("Parallel resistance of %lf and %lf\n", r1, r2);
    float ft1 = 1/r1;
    float ft2 = 1/r2;
    float result = 1 / (ft1 + ft2);
    eftsan_print_error(result);
    printf("Float result: %lf\n", result);
    return 0;
}
