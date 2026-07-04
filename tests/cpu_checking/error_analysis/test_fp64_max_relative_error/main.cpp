#include <cstdio>

FPC_CALCULATE_ERROR
void compute(volatile double *values, volatile double *sink, int n)
{
    double one = 1.0;
    for (int i = 0; i < n; ++i)
    {
        double value = values[i];
        double result = value + one; // TARGET_LINE
        *sink = result;
    }
}

int main()
{
    volatile double values[3] = {
        9007199254740992.0,
        18014398509481984.0,
        36028797018963968.0,
    };
    volatile double sink = 0.0;
    compute(values, &sink, 3);
    std::printf("sink: %.17e\n", sink);
    return 0;
}
