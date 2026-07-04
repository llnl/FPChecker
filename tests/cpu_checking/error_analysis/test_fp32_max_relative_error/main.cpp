#include <cstdio>

FPC_CALCULATE_ERROR
void compute(volatile float *values, volatile float *sink, int n)
{
    float one = 1.0f;
    for (int i = 0; i < n; ++i)
    {
        float value = values[i];
        float result = value + one; // TARGET_LINE
        *sink = result;
    }
}

int main()
{
    volatile float values[3] = {
        16777216.0f,
        33554432.0f,
        67108864.0f,
    };
    volatile float sink = 0.0f;
    compute(values, &sink, 3);
    std::printf("sink: %.9e\n", static_cast<double>(sink));
    return 0;
}
