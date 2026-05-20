#include <iostream>
#include <cmath>
#include <iomanip>

// ---------------- FP32 --------------------

//__attribute__((noinline)) __attribute__((annotate("_FPC_CALCULATE_ERROR_")))
void calc_dot_product_fma_f(const float *a, const float *b, size_t n, float &result)
{
    float res = 0.0f;

    for (size_t i = 0; i < n; ++i)
    {
        res = std::fma(a[i], b[i], res);
    }

    result = res;
}

__attribute__((noinline)) __attribute__((annotate("_FPC_CALCULATE_ERROR_"))) void calc_dot_product_f(const float *a, const float *b, size_t n, float &result)
{
    float res = 0.0f;
    for (size_t i = 0; i < n; ++i)
    {
        res += a[i] * b[i];
    }

    result = res;
}

// ---------------- FP64 --------------------

void calc_dot_product_fma_d(const double *a, const double *b, size_t n, double &result)
{
    double res = 0.0;

    for (size_t i = 0; i < n; ++i)
    {
        res = std::fma(a[i], b[i], res);
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

// ---------------- Initialization --------------------

static float initial_value_f = 0.03f;

// Initializes a float vector with a sequence: 0.03, 0.06, 0.09, ...
/*void initialize_vector_float(float *vec, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        vec[i] = initial_value_f + i * initial_value_f;
    }
}

static double initial_value_d = 0.03;

// Initializes a double vector with a sequence: 0.03, 0.06, 0.09, ...
void initialize_vector_double(double *vec, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        vec[i] = initial_value_d + i * initial_value_d;
    }
}*/

void initialize_vector_all(float *vec_f, double *vec_d, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        vec_f[i] = initial_value_f + i * initial_value_f;
        vec_d[i] = static_cast<double>(vec_f[i]);
    }
}

void side_by_side_product(const float *a_f, const float *b_f, size_t n, const double *a_d, const double *b_d)
{
    std::cout << "\n\n ====== Side-by-side product calculation =====\n"
              << std::endl;
    double res_d = 0.0;
    float res_f = 0.0f;
    double current_error = 0.0;

    std::cout << std::fixed << std::setprecision(17);
    std::cout << std::scientific;

    for (size_t i = 0; i < n; ++i)
    {
        // Computation in high precision
        // std::cout << "High precision input: " << a_d[i] << " " << b_d[i] << " " << res_d << std::endl;
        res_d = std::fma(a_d[i], b_d[i], res_d);
        // std::cout << "Result (high precision): " << res_d << std::endl;

        // Side-by-side computation
        // std::cout << "Current error: " << current_error << std::endl;
        double _a = static_cast<double>(a_f[i]);
        double _b = static_cast<double>(b_f[i]);
        double _c = static_cast<double>(res_f) + current_error;
        // std::cout << "Side-by-side input: " << _a << " " << _b << " " << _c << std::endl;
        double _tmp = std::fma(_a, _b, _c);
        // std::cout << "Result (side-by-side): " << _tmp << std::endl;

        // Float precision
        res_f = std::fma(a_f[i], b_f[i], res_f);
        // std::cout << "Result (float): " << res_f << std::endl;

        current_error = _tmp - static_cast<double>(res_f);
        // std::cout << "New error: " << current_error << "\n" << std::endl;
    }
    std::cout << "Final error: " << current_error << std::endl;
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
    initialize_vector_all(vec1, vec1_d, n);
    initialize_vector_all(vec2, vec2_d, n);

    // initialize_vector_float(vec1, n);
    // initialize_vector_float(vec2, n);
    // initialize_vector_double(vec1_d, n);
    // initialize_vector_double(vec2_d, n);

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

    // side_by_side_product(vec1, vec2, n, vec1_d, vec2_d);

    // Delete allocated memory
    delete[] vec1;
    delete[] vec2;
    delete[] vec1_d;
    delete[] vec2_d;

    return 0;
}