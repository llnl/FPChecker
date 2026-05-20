#include <iostream>
#include <cmath>
#include <iomanip>

// Chain several math operations in float precision.
// The accumulated rounding error should be detectable by FPChecker.
__attribute__((noinline))
float compute_f(float x)
{
    float r = sinf(x);        // line 10
    r = r + cosf(x);          // line 11
    r = sqrtf(fabsf(r));      // line 12
    r = expf(r);              // line 13
    r = logf(r);              // line 14
    r = powf(r, 2.0f);        // line 15
    return r;
}

// Same computation in double precision (ground truth).
__attribute__((noinline))
double compute_d(double x)
{
    double r = sin(x);
    r = r + cos(x);
    r = sqrt(fabs(r));
    r = exp(r);
    r = log(r);
    r = pow(r, 2.0);
    return r;
}

int main()
{
    float  input_f = 0.7f;
    double input_d = static_cast<double>(input_f);

    float  result_f = compute_f(input_f);
    double result_d = compute_d(input_d);

    double total_error = result_d - static_cast<double>(result_f);

    std::cout << std::scientific << std::setprecision(17);
    std::cout << "Result (float):  " << result_f << std::endl;
    std::cout << "Result (double): " << result_d << std::endl;
    std::cout << "Total Error: " << total_error << std::endl;

    return 0;
}
