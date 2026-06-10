
#include "api.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

const double FLOAT_SCALING_FACTOR = 2.123;

double compute_in_float(std::vector<double> values)
{
    std::vector<double> result(values.size(), 0.0);
    vector_addition(values, values, result);
    vector_scaling(result, FLOAT_SCALING_FACTOR, result);
    double dot_product_result = dot_product(result, result);
    return dot_product_result;
}

long double compute_in_double(std::vector<long double> values)
{
    std::vector<long double> result(values.size(), 0.0);
    vector_addition(values, values, result);
    vector_scaling(result, FLOAT_SCALING_FACTOR, result);
    long double dot_product_result = dot_product(result, result);
    return dot_product_result;
}

// Input is a vector in double precision: "0.1,0.2,0.3"
// We parse it and create a vector of floats
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <comma_separated_float_values>" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string input = argv[1];
    std::vector<double> values_f;
    std::vector<long double> values_d;
    std::stringstream ss(input);
    std::string item;

    while (std::getline(ss, item, ','))
    {
        try
        {
            values_f.push_back(std::stod(item));
            printf("Converted to double: %f\n", values_f.back());
            values_d.push_back(static_cast<long double>(values_f.back()));
        }
        catch (const std::invalid_argument &)
        {
            std::cerr << "Invalid double value: " << item << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // Print the parsed double values
    std::cout << "Parsed input double values:" << std::endl;
    for (const auto &val : values_f)
    {
        std::cout << val << std::endl;
    }

    double result_f = compute_in_float(values_f);
    long double result_d = compute_in_double(values_d);

    // set precision to 10 decimal places and use scientific notation
    std::cout.precision(17);
    std::cout << std::scientific;
    std::cout << "Computed result (double): " << result_f << std::endl;
    std::cout << "Computed result (long double): " << result_d << std::endl;

    // Difference
    long double difference = result_d - static_cast<long double>(result_f);
    std::cout << "Difference: " << difference << std::endl;

    // Relative error
    long double relative_error = 0.0;
    if (result_d != 0.0)
    {
        relative_error = std::abs(difference) / std::abs(result_d);
    }
    std::cout << "Relative error: " << relative_error << std::endl;

    return 0;
}