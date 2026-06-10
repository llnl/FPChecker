#include <iostream>
#include <cmath>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>

void calc_dot_product_fma_d(const long double *a, const long double *b, size_t n, long double &result)
{
    long double res = 0.0;

    for (size_t i = 0; i < n; ++i)
    {
        res = std::fma(a[i], b[i], res);
    }

    result = res;
}

// FPC_CALCULATE_ERROR
void calc_dot_product_fma_f(const double *a, const double *b, size_t n, double &result)
{
    double res = 0.0;

    for (size_t i = 0; i < n; ++i)
    {
        res = std::fma(a[i], b[i], res);
    }

    result = res;
}

// FPC_CALCULATE_ERROR
void calc_dot_product_f(const double *a, const double *b, size_t n, double &result)
{
    double res = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        res += a[i] * b[i];
    }

    result = res;
}

void calc_dot_product_d(const long double *a, const long double *b, size_t n, long double &result)
{
    long double res = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        res += a[i] * b[i];
    }

    result = res;
}

/* // {0.3, 0.6, 0.9, ..}
static double initial_value_f = 0.3;
void initialize_vector_all(double *vec_f, long double *vec_d, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        vec_f[i] = initial_value_f + i * initial_value_f;
        vec_d[i] = static_cast<long double>(vec_f[i]);
    }
} */

/* void initialize_vector_all_fixed(double *vec_f, long double *vec_d)
{
    const double initial_values[] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
    for (size_t i = 0; i < 10; ++i)
    {
        vec_f[i] = initial_values[i];
        vec_d[i] = static_cast<long double>(vec_f[i]);
    }
} */

// Example input:
//   "0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0"
int main(int argc, char *argv[])
{
    // ----
    // 1. The input string
    std::string input_str = argv[1];

    // The vector (dynamic array) to store the parsed double values
    std::vector<double> float_array;

    // --- Step 1: Pre-process the string (remove 'f' suffixes) ---
    // A simple way to do this is to replace all occurrences of "f," with ","
    size_t pos = input_str.find("f,");
    while (pos != std::string::npos)
    {
        input_str.replace(pos, 2, ","); // Replace "f," with just ","
        pos = input_str.find("f,", pos + 1);
    }
    // Handle the last element's 'f' (which doesn't have a trailing comma)
    if (input_str.back() == 'f')
    {
        input_str.pop_back();
    }

    // --- Step 2 & 3: Use stringstream for parsing ---
    std::stringstream ss(input_str);
    double value;
    char delimiter; // To hold the comma

    // The loop continues as long as a double value can be successfully extracted
    while (ss >> value)
    {
        float_array.push_back(value); // Store the extracted value

        // Try to extract the delimiter (the comma)
        // This is important to advance the stream past the delimiter
        // and its trailing space for the next loop iteration.
        ss >> delimiter;
        // Note: ss >> delimiter will skip leading whitespace, so it reads the ','
        // The next iteration's ss >> value will skip the space after the comma.
    }

    // --- Verification (Print the array contents) ---
    std::cout << "✅ Parsed values stored in array:\n";
    for (size_t i = 0; i < float_array.size(); ++i)
    {
        std::cout << "Index [" << i << "]: " << float_array[i] << "\n";
    }

    // --------------
    // const double initial_values[] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
    const double *initial_values = float_array.data();
    size_t n = float_array.size();

    // Allocate arrays
    double *vec1 = new double[n];
    double *vec2 = new double[n];
    long double *vec1_d = new long double[n];
    long double *vec2_d = new long double[n];

    for (size_t i = 0; i < 10; ++i)
    {
        vec1[i] = initial_values[i];
        vec1_d[i] = static_cast<long double>(vec1[i]);
        vec2[i] = initial_values[i];
        vec2_d[i] = static_cast<long double>(vec2[i]);
    }

    double result_f_normal;
    double result_f_fma;
    long double result_d_normal;
    long double result_d_fma;

    // Calculate dot products
    calc_dot_product_f(vec1, vec2, n, result_f_normal);
    calc_dot_product_fma_f(vec1, vec2, n, result_f_fma);
    calc_dot_product_d(vec1_d, vec2_d, n, result_d_normal);
    calc_dot_product_fma_d(vec1_d, vec2_d, n, result_d_fma);

    // Print results
    std::cout << std::fixed << std::setprecision(17);
    std::cout << std::scientific;
    std::cout << "Dot product (double, normal)  : " << result_f_normal << std::endl;
    std::cout << "Dot product (double, FMA)     : " << result_f_fma << std::endl;
    std::cout << "Dot product (long double, normal) : " << result_d_normal << std::endl;
    std::cout << "Dot product (long double, FMA)    : " << result_d_fma << std::endl;

    // Calculate the total rounding error
    long double total_error_normal = result_d_normal - static_cast<long double>(result_f_normal);
    long double total_error_fma = result_d_fma - static_cast<long double>(result_f_fma);

    std::cout << "\n--- Total Rounding Error ---" << std::endl;
    std::cout << "Total Error (normal): " << total_error_normal << std::endl;
    std::cout << "Total Error (FMA): " << total_error_fma << std::endl;

    // Delete allocated memory
    delete[] vec1;
    delete[] vec2;
    delete[] vec1_d;
    delete[] vec2_d;

    return 0;
}