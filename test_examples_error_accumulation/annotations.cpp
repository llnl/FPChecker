#include <stdio.h>

float compute(float *arr)
{
    float average = 0.0f;
    for (int i = 0; i < 3; ++i)
    {
        average += arr[i];
    }
    return average / 3.0f;
}

FPC_CALCULATE_ERROR
float accumulate(float a, float b)
{
    return a + b;
}

int main()
{
    float arr[3] = {1.0f, 2.0f, 3.0f};
    float avg = compute(arr);
    float result = accumulate(5.0f, 10.0f) + avg;
    printf("Result: %f\n", result);
    return 0;
}