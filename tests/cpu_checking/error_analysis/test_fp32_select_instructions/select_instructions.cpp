#include <iostream>

inline __attribute__((always_inline)) float max_value(float x, float y)
{
    // This will generate a select instruction in LLVM IR at -O2
    return (x > y) ? x : y;
}

FPC_CALCULATE_ERROR
void compute(float *a, float *b)
{
    // Apply the select logic in a loop for all elements except the first (to avoid out-of-bounds).
    for (int i = 1; i < 4; ++i)
    {
        a[i] = max_value(a[i - 1], b[i - 1]) * 1.3f;
    }
}

int main(int argc, char **argv)
{
    float a[4] = {1.3f, 2.3f, 4.3f, 5.3f};
    float b[4] = {6.3f, 5.3f, 6.3f, 7.3f};
    compute(a, b);
    for (int i = 0; i < 4; ++i)
    {
        std::cout << "a[" << i << "] = " << a[i] << ", b[" << i << "] = " << b[i] << std::endl;
    }
    return 0;
}