#include <iostream>

void performComputations_d(long double x, long double y, long double z, long double &result1, long double &result2)
{
    result1 = (x * 0.25) + (y * 0.5) + (z * 0.25);
    result2 = (x * x) * (y * y) * (z * z * z);
}

// This function performs two different computations on its inputs
// and returns two different floating-point values.
// The results are returned via reference parameters.
FPC_CALCULATE_ERROR void performComputations_f(double x, double y, double z, double &result1, double &result2)
{
    result1 = (x * 0.25) + (y * 0.5) + (z * 0.25);
    result2 = (x * x) * (y * y) * (z * z * z);
}

int main()
{
    double input1_f = 5.3;
    double input2_f = 10.3;
    double input3_f = 2.3;
    double output1_f;
    double output2_f;

    // Call the function to perform the computations.
    performComputations_f(input1_f, input2_f, input3_f, output1_f, output2_f);

    long double input1_d = (long double)input1_f;
    long double input2_d = (long double)input2_f;
    long double input3_d = (long double)input3_f;
    long double output1_d;
    long double output2_d;

    // Call the function to perform the computations.
    performComputations_d(input1_d, input2_d, input3_d, output1_d, output2_d);

    std::cout.precision(17);
    std::cout << std::scientific;
    // Print the results.
    std::cout << "Input values: " << input1_f << ", " << input2_f << ", " << input3_f << std::endl;
    std::cout << "Result 1 (Weighted Average): " << output1_f << std::endl;
    std::cout << "Result 2 (Complex Product): " << output2_f << std::endl;

    std::cout << "Total error 1: " << (output1_d - static_cast<long double>(output1_f)) << std::endl;
    std::cout << "Total error 2: " << (output2_d - static_cast<long double>(output2_f)) << std::endl;

    return 0;
}