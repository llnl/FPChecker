

#include <stdio.h>
#include <math.h>

FPC_CALCULATE_ERROR
void compute(float *arr, int size)
{
    arr[0] += 0.1f / arr[0]; // introduce some FP operation
    arr[1] = sinf(arr[0]);   // introduce some FP operation
    arr[3] = sqrtf(arr[3]);
    arr[4] = cosf(arr[5]);
    arr[6] = expf(arr[6]);
    arr[7] = logf(arr[7]);
}

int main()
{
    float arr[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
    compute(arr, 10);
    printf("Result: %.7f\n", arr[1]);
    printf("Result: %.7f\n", arr[3]);

    return 0;
}