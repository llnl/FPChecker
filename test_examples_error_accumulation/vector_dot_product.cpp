#include <iostream>
#include <cmath>
#include <iomanip>

void calc_dot_product_fma_d(const double *a, const double *b, size_t n, double &result)
{
    double res = 0.0;

    for (size_t i = 0; i < n; ++i)
    {
        res = std::fma(a[i], b[i], res);
    }

    result = res;
}

__attribute__((noinline)) __attribute__((annotate("_FPC_CALCULATE_ERROR_"))) void calc_dot_product_fma_f(const float *a, const float *b, size_t n, float &result)
{
    float res = 0.0f;

    for (size_t i = 0; i < n; ++i)
    {
        res = std::fma(a[i], b[i], res);
    }

    result = res;
}

//__attribute__((noinline)) __attribute__((annotate("_FPC_CALCULATE_ERROR_")))
void calc_dot_product_f(const float *a, const float *b, size_t n, float &result)
{
    float res = 0.0f;
    for (size_t i = 0; i < n; ++i)
    {
        res += a[i] * b[i];
    }

    result = res;
}

void calc_dot_product_d(const double *a, const double *b, size_t n, double &result)
{
    double res = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        res += a[i] * b[i];
    }

    result = res;
}

static float initial_value_f = 0.3f;

// Initializes a float vector with a sequence: 0.03, 0.06, 0.09, ...
void initialize_vector_float(float *vec, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        vec[i] = initial_value_f + i * initial_value_f;
    }
}
static double initial_value_d = 0.3;

// Initializes a double vector with a sequence: 0.03, 0.06, 0.09, ...
void initialize_vector_double(double *vec, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        vec[i] = initial_value_d + i * initial_value_d;
    }
}

int main(int argc, char *argv[])
{
    // Number of elements
    size_t n = (size_t)atoi(argv[1]);

    // Allocate arrays
    float *vec1 = new float[n];
    float *vec2 = new float[n];
    double *vec1_d = new double[n];
    double *vec2_d = new double[n];

    // Initialize arrays
    initialize_vector_float(vec1, n);
    initialize_vector_float(vec2, n);
    initialize_vector_double(vec1_d, n);
    initialize_vector_double(vec2_d, n);

    float result_f_normal;
    float result_f_fma;
    double result_d_normal;
    double result_d_fma;

    // Calculate dot products
    calc_dot_product_f(vec1, vec2, n, result_f_normal);
    calc_dot_product_fma_f(vec1, vec2, n, result_f_fma);
    calc_dot_product_d(vec1_d, vec2_d, n, result_d_normal);
    calc_dot_product_fma_d(vec1_d, vec2_d, n, result_d_fma);

    // Print results
    std::cout << std::fixed << std::setprecision(17);
    std::cout << std::scientific;
    std::cout << "Dot product (float, normal)  : " << result_f_normal << std::endl;
    std::cout << "Dot product (float, FMA)     : " << result_f_fma << std::endl;
    std::cout << "Dot product (double, normal) : " << result_d_normal << std::endl;
    std::cout << "Dot product (double, FMA)    : " << result_d_fma << std::endl;

    // Calculate the total rounding error
    double total_error_normal = result_d_normal - static_cast<double>(result_f_normal);
    double total_error_fma = result_d_fma - static_cast<double>(result_f_fma);

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