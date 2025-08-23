
#include <stdio.h>

FPC_CALCULATE_ERROR
void compute(float *a, float *b)
{
    a[0] = a[0] + b[0];
    a[1] = a[1] + b[1];
    b[0] = -b[1];
    b[2] = -b[3];
    b[4] = b[0] + b[2];
}

int main()
{
    float a[2] = {1.3f, 2.3f};
    float b[5] = {4.7f, 5.7f, 6.7f, 7.7f, 8.7f};

    compute(a, b);

    printf("Result a[0]: %f\n", a[0]);
    printf("Result a[1]: %f\n", a[1]);
    printf("Result b[0]: %f\n", b[0]);
    printf("Result b[1]: %f\n", b[1]);
    printf("Result b[2]: %f\n", b[2]);
    printf("Result b[3]: %f\n", b[3]);
    printf("Result b[4]: %f\n", b[4]);

    return 0;
}