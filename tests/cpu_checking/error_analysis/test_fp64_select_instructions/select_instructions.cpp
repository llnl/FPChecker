#include <iostream>

inline __attribute__((always_inline)) double max_value(double x, double y)
{
    // This will generate a select instruction in LLVM IR at -O2
    return (x > y) ? x : y;
}

FPC_CALCULATE_ERROR
void compute(double *a, double *b)
{
    // Apply the select logic in a loop for all elements except the first (to avoid out-of-bounds).
    for (int i = 1; i < 4; ++i)
    {
        a[i] = max_value(a[i - 1], b[i - 1]) * 1.3;
    }
}

int main(int argc, char **argv)
{
    double a[4] = {1.3, 2.3, 4.3, 5.3};
    double b[4] = {6.3, 5.3, 6.3, 7.3};
    compute(a, b);
    for (int i = 0; i < 4; ++i)
    {
        std::cout << "a[" << i << "] = " << a[i] << ", b[" << i << "] = " << b[i] << std::endl;
    }
    return 0;
}