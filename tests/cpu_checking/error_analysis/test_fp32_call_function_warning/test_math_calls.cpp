#include <cmath>
#include <cstdio>

FPC_CALCULATE_ERROR
float foo(float *arr, size_t n)
{
    float ret = 0.0f;
    for (size_t i = 0; i < n; ++i)
    {
        ret += arr[i] * 0.5f;
        ret += std::sin(arr[i]);
    }
    return ret;
}

int main()
{
    float arr[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float result = foo(arr, 4);
    printf("Result: %.6f\n", result);
    return 0;
}