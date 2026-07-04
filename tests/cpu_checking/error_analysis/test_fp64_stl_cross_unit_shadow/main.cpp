#include "api.h"

#include <iomanip>
#include <iostream>
#include <vector>

FPC_CALCULATE_ERROR
double compute_double()
{
    std::vector<double> values = {
        0.1111111111111111, 2.1111111111111112, 3.1111111111111112,
        40.111111111111114, 50.111111111111114, 60.111111111111114,
        7000.1111111111113};
    std::vector<double> transformed = transform_values(values);
    double reduced = consume_values(transformed);
    double result = reduced + 0.125; // FINAL_RESULT
    return result;
}

long double compute_long_double()
{
    std::vector<double> input = {
        0.1111111111111111, 2.1111111111111112, 3.1111111111111112,
        40.111111111111114, 50.111111111111114, 60.111111111111114,
        7000.1111111111113};
    std::vector<long double> values;
    values.reserve(input.size());
    for (double value : input)
    {
        values.push_back(static_cast<long double>(value));
    }

    std::vector<long double> transformed = transform_values(values);
    long double reduced = consume_values(transformed);
    return reduced + static_cast<long double>(0.125);
}

int main()
{
    double result = compute_double();
    long double reference = compute_long_double();
    long double difference = reference - static_cast<long double>(result);

    std::cout << std::scientific << std::setprecision(21);
    std::cout << "Result: " << result << "\n";
    std::cout << "Reference: " << reference << "\n";
    std::cout << "Difference: " << difference << "\n";
    return 0;
}
