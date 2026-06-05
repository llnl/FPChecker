#include <iostream>
#include <cmath>
#include <iomanip>

// Chain several math operations in double precision.
// The accumulated rounding error should be detectable by FPChecker.
__attribute__((noinline))
double compute_d(double x)
{
    double r = sin(x);        // line 10
    r = r + cos(x);          // line 11
    r = sqrt(fabs(r));      // line 12
    r = exp(r);              // line 13
    r = log(r);              // line 14
    r = pow(r, 2.0);        // line 15
    return r;
}

// Same computation in long double precision (ground truth).
__attribute__((noinline))
long double compute_ld(long double x)
{
    long double r = sinl(x);
    r = r + cosl(x);
    r = sqrtl(fabs(r));
    r = expl(r);
    r = logl(r);
    r = powl(r, 2.0);
    return r;
}

int main()
{
    double  input_d = 0.17;
    long double input_ld = static_cast<long double>(input_d);

    double  result_d = compute_d(input_d);
    long double result_ld = compute_ld(input_ld);

    long double total_error = result_ld - static_cast<long double>(result_d);

    std::cout << std::scientific << std::setprecision(17);
    std::cout << "Result (float):  " << result_d << std::endl;
    std::cout << "Result (double): " << result_d << std::endl;
    std::cout << "Result (long double): " << result_ld << std::endl;
    std::cout << "Total Error: " << total_error << std::endl;

    return 0;
}
