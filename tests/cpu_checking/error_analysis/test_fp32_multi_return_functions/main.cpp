#include <iostream>
#include <cmath>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>

// Elementary operations for ADD and MUL functions
__attribute__((noinline)) float add_f(float a, float b)
{
    return a + b;
}

__attribute__((noinline)) float mul_f(float a, float b)
{
    return a * b;
}

__attribute__((noinline))
void calc_dot_product_f(const float *a, const float *b, size_t n, float &result)
{
    float res = 0.0f;
    for (size_t i = 0; i < n; ++i)
    {
        res = add_f(res, mul_f(a[i], b[i]));
    }

    result = res;
}

__attribute__((noinline))
void calc_dot_product_d(const double *a, const double *b, size_t n, double &result)
{
    double res = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        res += a[i] * b[i];
    }

    result = res;
}

// Example input:
//   "0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f"
int main(int argc, char *argv[])
{    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <comma-separated list of float values with 'f' suffix>\n";
        std::cerr << "Example: " << argv[0] << " \"0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f\"\n";
        return 1;
    }
    std::string input_str = argv[1];

    // The vector (dynamic array) to store the parsed float values
    std::vector<float> float_array;

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
    float value;
    char delimiter; // To hold the comma

    // The loop continues as long as a float value can be successfully extracted
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
    // const float initial_values[] = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f};
    const float *initial_values = float_array.data();
    size_t n = float_array.size();

    // Allocate arrays
    float *vec1 = new float[n];
    float *vec2 = new float[n];
    double *vec1_d = new double[n];
    double *vec2_d = new double[n];

    for (size_t i = 0; i < 10; ++i)
    {
        vec1[i] = initial_values[i];
        vec1_d[i] = static_cast<double>(vec1[i]);
        vec2[i] = initial_values[i];
        vec2_d[i] = static_cast<double>(vec2[i]);
    }

    float result_f = 0.0f;
    double result_d = 0.0;

    // Calculate dot products
    calc_dot_product_f(vec1, vec2, n, result_f);
    calc_dot_product_d(vec1_d, vec2_d, n, result_d);

    // Print results
    std::cout << std::fixed << std::setprecision(17);
    std::cout << std::scientific;
    std::cout << "Dot product (float)  : " << result_f << std::endl;
    std::cout << "Dot product (double) : " << result_d << std::endl;

    // Calculate the total rounding error
    double total_error_normal = result_d - static_cast<double>(result_f);

    std::cout << "\n--- Total Rounding Error ---" << std::endl;
    std::cout << "Total Error (normal): " << total_error_normal << std::endl;

    // Delete allocated memory
    delete[] vec1;
    delete[] vec2;
    delete[] vec1_d;
    delete[] vec2_d;

    return 0;
}