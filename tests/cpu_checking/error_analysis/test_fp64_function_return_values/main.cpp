#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>

__attribute__((noinline)) double callee_fp64(double a, double b)
{
    double t = a * b;
    t = t + 0.1234567;
    t = t * 1.0000001;
    return t;
}

__attribute__((noinline)) long double callee_fp64(long double a, long double b)
{
    long double t = a * b;
    t = t + 0.1234567;
    t = t * 1.0000001;
    return t;
}

int main(int argc, char *argv[])
{
    double a = 0.1234567;
    double b = 2.7654321;

    if (argc >= 3)
    {
        a = std::strtod(argv[1], nullptr);
        b = std::strtod(argv[2], nullptr);
    }

    using fn_t = double (*)(double, double);
    fn_t selected = callee_fp64;

    // Keep indirection explicit so the call result must propagate through
    // function-return tracking rather than direct compile-time call matching.
    volatile fn_t fp = selected;
    double r = fp(a, b);
    double out = (r * 3.1415927) + 0.33333334;

    long double rd = callee_fp64((long double)a, (long double)b);
    long double outd = (rd * 3.141592653589793) + 0.3333333333333333;

    std::cout << std::fixed << std::setprecision(17) << std::scientific;
    std::cout << "Caller output (double):  " << out << std::endl;
    std::cout << "Caller output (long double): " << outd << std::endl;

    long double diff = outd - (long double)out;
    std::cout << "Difference: " << diff << std::endl;

    return 0;
}
