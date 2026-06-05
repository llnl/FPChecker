#include <iostream>
#include <cstddef>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>

__attribute__((noinline)) void scale_double_values(const double *input, double *output, size_t size, bool use_volatile = true)
{
    double scale = input[0];

    if (!use_volatile)
    {
        // Option A — make the temp array volatile (GCC extension VLA kept as in your code)
        volatile double tmp_values[size];
        for (size_t i = 0; i < size; i++)
        {
            tmp_values[i] = input[i] * scale;
            printf("Scaled value %zu: %.17e\n", i, tmp_values[i]);
        }
        for (size_t i = 0; i < size; i++)
        {
            output[i] = tmp_values[i]; // volatile reads prevent memcpy optimization
        }
    }
    else
    { // Option B — use memmove
        double tmp_values[size];
        for (size_t i = 0; i < size; i++)
        {
            tmp_values[i] = input[i] * scale;
            printf("(MEMMOVE) Scaled value %zu: %.17e\n", i, tmp_values[i]);
        }
        std::memmove(&output[1], &tmp_values[0], size * sizeof(double));
    }
}

__attribute__((noinline)) void scale_double_values_ld(const long double *input, long double *output, size_t size)
{
    long double tmp_values[size];
    long double scale = static_cast<long double>(input[0]);
    for (size_t i = 0; i < size; i++)
    {
        tmp_values[i] = input[i] * scale;
    }
    std::memmove(&output[1], &tmp_values[0], size * sizeof(long double));
}

__attribute__((noinline)) double sum_of_elements(const double *array, size_t n)
{
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        sum += array[i];
    }
    return sum;
}

__attribute__((noinline))long double sum_of_elements_ld(const long double *array, size_t n)
{
    long double sum = 0.0L;
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
        std::cerr << "Usage: " << argv[0] << " <comma_separated_double_values>" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string input = argv[1];
    std::vector<double> values_d;
    std::stringstream ss(input);
    std::string item;

    while (std::getline(ss, item, ','))
    {
        try
        {
            values_d.push_back(std::stod(item));
        }
        catch (const std::invalid_argument &)
        {
            std::cerr << "Invalid double value: " << item << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // Print the parsed double values
    std::cout << "Parsed input double values:" << std::endl;
    for (const auto &val : values_d)
        std::cout << val << std::endl;

    size_t size = values_d.size();
    double output_values[size + 1];
    scale_double_values(values_d.data(), output_values, size, true);

    double result = 0.0;
    // calc_dot_product_d(output_values, output_values, size, result);
    result = sum_of_elements(&output_values[1], size);
    printf("Result (double): %.17e\n", result);

    // Compute in long double for reference
    std::vector<long double> values_ld;
    for (size_t i = 0; i < values_d.size(); i++)
    {
        values_ld.push_back(static_cast<long double>(values_d[i]));
    }

    size_t size_d = values_d.size();
    long double output_values_d[size_d + 1];
    scale_double_values_ld(values_ld.data(), output_values_d, size_d);

    long double result_d = 0.0L;
    // calc_dot_product_f(output_values, output_values, size, result);
    result_d = sum_of_elements_ld(&output_values_d[1], size_d);
    printf("Result (long double): %.21Le\n", result_d);

    long double difference = result_d - static_cast<long double>(result);
    printf("Difference: %.21Le\n", difference);

    return 0;
}