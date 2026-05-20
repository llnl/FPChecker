#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>

__attribute__((noinline)) float callee_fp32(float a, float b)
{
    float t = a * b;
    t = t + 0.1234567f;
    t = t * 1.0000001f;
    return t;
}

__attribute__((noinline)) double callee_fp64(double a, double b)
{
    double t = a * b;
    t = t + 0.1234567;
    t = t * 1.0000001;
    return t;
}

int main(int argc, char *argv[])
{
    float a = 0.1234567f;
    float b = 2.7654321f;

    if (argc >= 3)
    {
        a = std::strtof(argv[1], nullptr);
        b = std::strtof(argv[2], nullptr);
    }

    using fn_t = float (*)(float, float);
    fn_t selected = callee_fp32;

    // Keep indirection explicit so the call result must propagate through
    // function-return tracking rather than direct compile-time call matching.
    volatile fn_t fp = selected;
    float r = fp(a, b);
    float out = (r * 3.1415927f) + 0.33333334f;

    double rd = callee_fp64((double)a, (double)b);
    double outd = (rd * 3.141592653589793) + 0.3333333333333333;

    std::cout << std::fixed << std::setprecision(17) << std::scientific;
    std::cout << "Caller output (float):  " << out << std::endl;
    std::cout << "Caller output (double): " << outd << std::endl;

    double diff = outd - (double)out;
    std::cout << "Difference: " << diff << std::endl;

    return 0;
}
