
#include <cstdlib>
#include <iostream>
#include <cmath>
#include <iomanip>

int main(int argc, char **argv)
{
    // Get 4 values from input
    float a_f = atof(argv[1]);
    float b_f = atof(argv[2]);
    float c_f = atof(argv[3]);
    double error = atof(argv[4]);

    std::cout << std::fixed << std::setprecision(17);
    std::cout << std::scientific;

    std::cout << "Input values: " << a_f << ", " << b_f << ", " << c_f << ", " << error << std::endl;

    // Compute FMA
    float fma_result = fma(a_f, b_f, c_f);

    double a_d = static_cast<double>(a_f);
    double b_d = static_cast<double>(b_f);
    double c_d = static_cast<double>(c_f);
    c_d += error;
    std::cout << "c after error: " << c_d << std::endl;

    double fma_result_d = fma(a_d, b_d, c_d);

    std::cout << "FMA Result (float)         : " << fma_result << std::endl;
    std::cout << "FMA Result (float, casted) : " << static_cast<double>(fma_result) << std::endl;
    std::cout << "FMA Result (double)        : " << fma_result_d << std::endl;

    double total_error = fma_result_d - static_cast<double>(fma_result);
    std::cout << "Total error: " << total_error << std::endl;

    return 0;
}