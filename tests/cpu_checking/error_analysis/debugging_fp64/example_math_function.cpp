#include <stdio.h>
#include <math.h>


__attribute__((noinline))
double compute_d(double x)
{
    double r = sin(x);
    r = r + cos(x);
    r = sqrt(fabs(r));
    r = exp(r);
    r = log(r);
    r = pow(r, 2.0);
    return r;
}

/*
   Same computation in long double precision.
*/
__attribute__((noinline))
long double compute_ld(long double x)
{
    long double r = sinl(x);
    r = r + cosl(x);
    r = sqrtl(fabsl(r));
    r = expl(r);
    r = logl(r);
    r = powl(r, 2.0L);
    return r;
}

int main(void)
{
    /*
       Same mathematical input value,
       represented in different precision types.
    */
    double input_d = 1.7;
    long double input_ld = 1.7L;

    double result_d = compute_d(input_d);
    long double result_ld = compute_ld(input_ld);


    long double diff = result_ld - (long double)result_d;


    printf("--- RESULTS ---\n");
    printf("Result(Double):      %.17f\n", result_d);
    printf("Result(Long Double): %.21Lf\n", result_ld);

    printf("\n--- PRECISION DIFFERENCES ---\n");
    printf("Double to long double precision: %.21Le\n", diff);

    return 0;
}