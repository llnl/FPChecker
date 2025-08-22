
#include <stdio.h>

FPC_CALCULATE_ERROR
float compute(float *a, float *b, float *c)
{
    float tmp_1 = a[0] / b[0];
    float tmp_2 = a[1] + b[1];
    c[0] = tmp_1 * tmp_2;
    c[1] = tmp_1 - tmp_2 + c[0];
    c[2] = (c[0] / c[1]) / (tmp_1 + tmp_2);

    return c[2];
}

int main()
{
    float a[2] = {1.3f, 2.3f};
    float b[2] = {2.3f, 3.3f};
    float c[3] = {0.0f, 0.0f, 0.0f};

    float result = compute(a, b, c);

    printf("Result: %f\n", result);
    printf("Result c[0]: %f\n", c[0]);
    printf("Result c[1]: %f\n", c[1]);
    printf("Result c[2]: %f\n", c[2]);
    return 0;
}