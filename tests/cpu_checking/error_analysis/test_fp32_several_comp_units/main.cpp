
#include "api.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

const float FLOAT_SCALING_FACTOR = 2.123f;

float compute_in_float(std::vector<float> values)
{
    std::vector<float> result(values.size(), 0.0f);
    vector_addition(values, values, result);
    vector_scaling(result, FLOAT_SCALING_FACTOR, result);
    float dot_product_result = dot_product(result, result);
    return dot_product_result;
}

double compute_in_double(std::vector<double> values)
{
    std::vector<double> result(values.size(), 0.0);
    vector_addition(values, values, result);
    vector_scaling(result, FLOAT_SCALING_FACTOR, result);
    double dot_product_result = dot_product(result, result);
    return dot_product_result;
}

// Input is a vector in float precision: "0.1f,0.2f,0.3f"
// We parse it and create a vector of floats
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <comma_separated_float_values>" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string input = argv[1];
    std::vector<float> values_f;
    std::vector<double> values_d;
    std::stringstream ss(input);
    std::string item;

    while (std::getline(ss, item, ','))
    {
        try
        {
            values_f.push_back(std::stof(item));
            printf("Converted to float: %f\n", values_f.back());
            values_d.push_back(static_cast<double>(values_f.back()));
        }
        catch (const std::invalid_argument &)
        {
            std::cerr << "Invalid float value: " << item << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // Print the parsed float values
    std::cout << "Parsed input float values:" << std::endl;
    for (const auto &val : values_f)
    {
        std::cout << val << std::endl;
    }

    float result_f = compute_in_float(values_f);
    double result_d = compute_in_double(values_d);

    // set precision to 10 decimal places and use scientific notation
    std::cout.precision(17);
    std::cout << std::scientific;
    std::cout << "Computed result (float): " << result_f << std::endl;
    std::cout << "Computed result (double): " << result_d << std::endl;

    // Difference
    double difference = result_d - static_cast<double>(result_f);
    std::cout << "Difference: " << difference << std::endl;

    // Relative error
    double relative_error = 0.0;
    if (result_d != 0.0)
    {
        relative_error = std::abs(difference) / std::abs(result_d);
    }
    std::cout << "Relative error: " << relative_error << std::endl;

    return 0;
}