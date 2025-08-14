#include <iostream>
#include <cmath>

void calc_dot_product_fma_f(const float *a, const float *b, size_t n, float &result)
{
    float res = 0.0f;
    double res_i = 0.0;

    for (size_t i = 0; i < n; ++i)
    {
        // Implement in double precision
        double a_i = static_cast<double>(a[i]);
        double b_i = static_cast<double>(b[i]);

        res = std::fma(a[i], b[i], res);
        res_i = std::fma(a_i, b_i, res_i);

        printf("FMA (single): a[%zu]=%.7e, b[%zu]=%.7e, res=%.7e\n", i, a[i], i, b[i], res);
        printf("FMA (double): a[%zu]=%.17e, b[%zu]=%.17e, res=%.17e\n", i, a_i, i, b_i, res_i);
        printf("Error: %.17e\n", res_i - static_cast<double>(res));
    }

    printf("\t ==== Final Error in FMA function: %.17e\n", res_i - static_cast<double>(res));

    result = res;
}

// FPC_CALCULATE_ERROR
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

// Initializes a float vector with a sequence: 0.03, 0.06, 0.09, ...
void initialize_vector_float(float *vec, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        vec[i] = 0.03f + i * 0.03f;
    }
}

// Initializes a double vector with a sequence: 0.03, 0.06, 0.09, ...
void initialize_vector_double(double *vec, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        vec[i] = 0.03 + i * 0.03;
    }
}

int main(int argc, char *argv[])
{
    size_t n = (size_t)atoi(argv[1]);
    float *vec1 = new float[n];
    float *vec2 = new float[n];
    initialize_vector_float(vec1, n);
    initialize_vector_float(vec2, n);
    float result_f;
    calc_dot_product_f(vec1, vec2, n, result_f);

    calc_dot_product_fma_f(vec1, vec2, n, result_f);

    std::cout << "Dot product (float): " << result_f << std::endl;

    double *vec1_d = new double[n];
    double *vec2_d = new double[n];
    initialize_vector_double(vec1_d, n);
    initialize_vector_double(vec2_d, n);
    double result_d;
    calc_dot_product_d(vec1_d, vec2_d, n, result_d);
    std::cout << "Dot product (double): " << result_d << std::endl;

    // ------ Average ------------
    // Calculate the total rounding error
    double total_error = result_d - static_cast<double>(result_f);

    std::cout << "--- Total Rounding Error ---" << std::endl;
    // Show the error in scientific notation for clarity
    // std::cout << std::scientific;
    // std::cout << "Total Error (sci): " << total_error << std::endl;
    printf("Total Error: %.17e\n", total_error);

    return 0;
}