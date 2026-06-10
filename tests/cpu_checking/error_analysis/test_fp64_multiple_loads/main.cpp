
#include <stdio.h>

FPC_CALCULATE_ERROR
double compute(double *a, double *b, double *c)
{
    double tmp_1 = a[0] / b[0];
    double tmp_2 = a[1] + b[1];
    c[0] = tmp_1 * tmp_2;
    c[1] = tmp_1 - tmp_2 + c[0];
    c[2] = (c[0] / c[1]) / (tmp_1 + tmp_2);

    return c[2];
}

int main()
{
    double a[2] = {1.3, 2.3};
    double b[2] = {2.3, 3.3};
    double c[3] = {0.0, 0.0, 0.0};

    double result = compute(a, b, c);

    printf("Result: %f\n", result);
    printf("Result c[0]: %f\n", c[0]);
    printf("Result c[1]: %f\n", c[1]);
    printf("Result c[2]: %f\n", c[2]);
    return 0;
}