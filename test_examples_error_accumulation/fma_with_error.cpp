
#include <cstdlib>
#include <iostream>
#include <cmath>
#include <iomanip>

int main(int argc, char **argv)
{
    std::cout << std::scientific;

    float a_f = 1.0375000e+00;
    float b_f = 2.0375001e+00;
    float c_f = -2.1139064e+00;

    double a_d = (double)a_f;
    double b_d = (double)b_f;
    double c_d = (double)c_f;

    // Values before error
    std::cout << std::setprecision(17);
    std::cout << "a_d : " << a_d << std::endl;
    std::cout << "b_d : " << b_d << std::endl;
    std::cout << "c_d : " << c_d << std::endl;

    double a_d_error = a_d + -2.98023223876953125e-08;
    double b_d_error = b_d + -2.98023223876953125e-08;
    double c_d_error = c_d + 2.81631953313876693e-08;

    // Values after error
    std::cout << std::setprecision(17);
    std::cout << "a_d_error : " << a_d_error << std::endl;
    std::cout << "b_d_error : " << b_d_error << std::endl;
    std::cout << "c_d_error : " << c_d_error << std::endl;

    double fma_d = fma(a_d, b_d, c_d);
    double fma_d_error = fma(a_d_error, b_d_error, c_d_error);
    float fma_f = fmaf(a_f, b_f, c_f);

    std::cout << std::setprecision(7);
    std::cout << "FMA Result (float)         : " << fma_f << std::endl;
    std::cout << std::setprecision(17);
    std::cout << "FMA Result (double)        : " << fma_d << std::endl;
    std::cout << "FMA Result (double, error) : " << fma_d_error << std::endl;

    return 0;
}