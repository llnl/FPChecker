#include <iostream>
#include <vector>

// Multiply matrix A (m x n) with vector x (n), result is vector y (m)
// FPC_CALCULATE_ERROR
void matrix_vector_multiply(const float *A, const float *x, float *y, int m, int n)
{
    for (int i = 0; i < m; ++i)
    {
        y[i] = 0.0;
        for (int j = 0; j < n; ++j)
        {
            y[i] += A[i * n + j] * x[j];
        }
    }
}

int main()
{
    // Example: 3x2 matrix and 2-element vector
    int m = 3, n = 2;
    float A[] = {1.0f, 2.0f,
                 3.0f, 4.0f,
                 5.0f, 6.0f}; // Row-major: 3 rows, 2 cols
    float x[] = {0.5f, -1.0f};
    float y[3];

    matrix_vector_multiply(A, x, y, m, n);

    std::cout << "Result y = Ax:\n";
    for (int i = 0; i < m; ++i)
    {
        std::cout << y[i] << " ";
    }
    std::cout << std::endl;

    return 0;
}