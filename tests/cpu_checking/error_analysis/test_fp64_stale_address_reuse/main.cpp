#include <iomanip>
#include <iostream>

extern "C" void overwrite_double(double *dst, double value);

FPC_CALCULATE_ERROR
double compute_with_external_overwrite()
{
    double a = 0.1;
    double b = 0.2;
    double slot = a * b;

    overwrite_double(&slot, 7.25);

    double loaded = slot;
    double scale = 1.1;
    double bias = 0.33333333333333331;
    double result = (loaded * scale) + bias; // FINAL_RESULT
    return result;
}

long double compute_reference()
{
    double loaded_d = 7.25;
    double scale_d = 1.1;
    double bias_d = 0.33333333333333331;
    return (static_cast<long double>(loaded_d) * static_cast<long double>(scale_d)) +
           static_cast<long double>(bias_d);
}

int main()
{
    double result = compute_with_external_overwrite();
    long double reference = compute_reference();
    long double difference = reference - static_cast<long double>(result);

    std::cout << std::scientific << std::setprecision(21);
    std::cout << "Result: " << result << "\n";
    std::cout << "Reference: " << reference << "\n";
    std::cout << "Difference: " << difference << "\n";
    return 0;
}
