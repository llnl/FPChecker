#include "api.h"

#include <iomanip>
#include <iostream>
#include <vector>

FPC_CALCULATE_ERROR
float compute_float()
{
    std::vector<float> values = {
        0.11111111f, 2.1111112f, 3.1111112f, 40.111111f,
        50.111111f, 60.111111f, 7000.1113f};
    std::vector<float> transformed = transform_values(values);
    float reduced = consume_values(transformed);
    float result = reduced + 0.125f; // FINAL_RESULT
    return result;
}

double compute_double()
{
    std::vector<float> input = {
        0.11111111f, 2.1111112f, 3.1111112f, 40.111111f,
        50.111111f, 60.111111f, 7000.1113f};
    std::vector<double> values;
    values.reserve(input.size());
    for (float value : input)
    {
        values.push_back(static_cast<double>(value));
    }

    std::vector<double> transformed = transform_values(values);
    double reduced = consume_values(transformed);
    return reduced + static_cast<double>(0.125f);
}

int main()
{
    float result = compute_float();
    double reference = compute_double();
    double difference = reference - static_cast<double>(result);

    std::cout << std::scientific << std::setprecision(17);
    std::cout << "Result: " << result << "\n";
    std::cout << "Reference: " << reference << "\n";
    std::cout << "Difference: " << difference << "\n";
    return 0;
}
