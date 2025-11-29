
#include <iostream>
#include <cstddef>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>

__attribute__((noinline)) void scale_float_values(const float *input, float *output, size_t size, bool use_volatile = true)
{
    float scale = input[0];

    if (!use_volatile)
    {
        // Option A — make the temp array volatile (GCC extension VLA kept as in your code)
        volatile float tmp_values[size];
        for (size_t i = 0; i < size; i++)
        {
            tmp_values[i] = input[i] * scale;
            printf("Scaled value %zu: %.6f\n", i, tmp_values[i]);
        }
        for (size_t i = 0; i < size; i++)
        {
            output[i] = tmp_values[i]; // volatile reads prevent memcpy optimization
        }
    }
    else
    { // Option B — use memcpy
        float tmp_values[size];
        for (size_t i = 0; i < size; i++)
        {
            tmp_values[i] = input[i] * scale;
            printf("(MEMMOVE) Scaled value %zu: %.6f\n", i, tmp_values[i]);
        }
        std::memmove(&output[1], &tmp_values[0], size * sizeof(float));
    }
}

__attribute__((noinline)) void scale_float_values_d(const double *input, double *output, size_t size)
{
    double tmp_values[size];
    double scale = static_cast<double>(input[0]);
    for (size_t i = 0; i < size; i++)
    {
        tmp_values[i] = input[i] * scale;
    }
    std::memmove(&output[1], &tmp_values[0], size * sizeof(double));
}

__attribute__((noinline)) float sum_of_elements(const float *array, size_t n)
{
    float sum = 0.0f;
    for (size_t i = 0; i < n; ++i)
    {
        sum += array[i];
    }
    return sum;
}

__attribute__((noinline)) double sum_of_elements_d(const double *array, size_t n)
{
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        sum += array[i];
    }
    return sum;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <comma_separated_float_values>" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string input = argv[1];
    std::vector<float> values_f;
    std::stringstream ss(input);
    std::string item;

    while (std::getline(ss, item, ','))
    {
        try
        {
            values_f.push_back(std::stof(item));
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
        std::cout << val << std::endl;

    size_t size = values_f.size();
    float output_values[size + 1];
    scale_float_values(values_f.data(), output_values, size, true);

    float result = 0.0f;
    // calc_dot_product_f(output_values, output_values, size, result);
    result = sum_of_elements(&output_values[1], size);
    printf("Result (float): %.17f\n", result);

    // Compute in FP64 for reference
    std::vector<double> values_d;
    for (size_t i = 0; i < values_f.size(); i++)
    {
        values_d.push_back(static_cast<double>(values_f[i]));
    }

    size_t size_d = values_d.size();
    double output_values_d[size_d + 1];
    scale_float_values_d(values_d.data(), output_values_d, size_d);

    double result_d = 0.0;
    // calc_dot_product_f(output_values, output_values, size, result);
    result_d = sum_of_elements_d(&output_values_d[1], size_d);
    printf("Result (double): %.17f\n", result_d);

    double difference = result_d - static_cast<double>(result);
    printf("Difference: %.17e\n", difference);

    return 0;
}