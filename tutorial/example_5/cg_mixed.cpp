#include <iostream>
#include <vector>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <iomanip>
#include <chrono>

using namespace std;

static std::chrono::high_resolution_clock::time_point start_time, end_time;

void start_timer()
{
    start_time = std::chrono::high_resolution_clock::now();
}

void stop_timer()
{
    end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    cout << "Execution time: " << elapsed.count() << " seconds" << endl;
}

inline __attribute__((always_inline)) float my_sqrt(float x)
{
    if (x < 0.0f)
    {
        std::cout << "sqrt: negative input" << std::endl;
        return -1.0f; // Meaningful value indicating error
    }

    if (x == 0.0f || x == 1.0f)
        return x;

    float guess = x / 2.0f;
    for (int i = 0; i < 20; ++i)
    {
        guess = 0.5f * (guess + x / guess);
    }
    return guess;
}

inline __attribute__((always_inline)) double my_sqrt(double x)
{
    if (x < 0.0)
    {
        std::cout << "sqrt: negative input" << std::endl;
        return -1.0f; // Meaningful value indicating error
    }

    if (x == 0.0 || x == 1.0)
        return x;

    double guess = x / 2.0;
    for (int i = 0; i < 20; ++i)
    {
        guess = 0.5 * (guess + x / guess);
    }
    return guess;
}

// --- Vector and Matrix Operations ---

// Dot Product (v1^T * v2)
inline __attribute__((always_inline)) float dot_product(const vector<float> &v1, const vector<float> &v2)
{
    float result = 0.0f;
    for (size_t i = 0; i < v1.size(); ++i)
    {
        result += v1[i] * v2[i];
    }
    return result;
}

inline __attribute__((always_inline)) double dot_product_FP64(const vector<float> &v1, const vector<float> &v2)
{
    double result = 0.0;
    for (size_t i = 0; i < v1.size(); ++i)
    {
        result += static_cast<double>(v1[i]) * static_cast<double>(v2[i]);
    }
    return result;
}

inline __attribute__((always_inline)) double dot_product_FP64(const vector<double> &v1, const vector<double> &v2)
{
    double result = 0.0;
    for (size_t i = 0; i < v1.size(); ++i)
    {
        result += v1[i] * v2[i];
    }
    return result;
}

// Matrix-Vector Multiplication (A * v)
// A is stored as a flattened array (row-major order).
inline __attribute__((always_inline))
vector<float>
mat_vec_mult(const vector<float> &A, const vector<float> &v, size_t n)
{
    if (A.size() != n * n)
    {
        std::cout << "Matrix size does not match vector size." << std::endl;
        exit(1);
    }

    vector<float> result(n, 0.0f);
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = 0; j < n; ++j)
        {
            // A[i][j] is stored at index i * n + j
            result[i] += A[i * n + j] * v[j];
        }
    }
    return result;
}

inline __attribute__((always_inline))
vector<float>
mat_vec_mult(const vector<float> &A, const vector<double> &v, size_t n)
{
    if (A.size() != n * n)
    {
        std::cout << "Matrix size does not match vector size." << std::endl;
        exit(1);
    }

    vector<float> result(n, 0.0f);
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = 0; j < n; ++j)
        {
            // A[i][j] is stored at index i * n + j
            result[i] += A[i * n + j] * v[j];
        }
    }
    return result;
}

inline __attribute__((always_inline))
vector<float>
mat_vec_mult_FP64(const vector<float> &A, const vector<float> &v, size_t n)
{
    if (A.size() != n * n)
    {
        std::cout << "Matrix size does not match vector size." << std::endl;
        exit(1);
    }

    vector<float> result(n, 0.0f);
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = 0; j < n; ++j)
        {
            // A[i][j] is stored at index i * n + j
            result[i] = static_cast<double>(result[i]) + static_cast<double>(A[i * n + j]) * static_cast<double>(v[j]);
        }
    }
    return result;
}

// Vector Addition/Subtraction (v1 - alpha * v2 or v1 + alpha * v2)
inline __attribute__((always_inline)) vector<float>
vec_add_mult(const vector<float> &v1, const vector<float> &v2, float alpha, bool subtract = false)
{
    vector<float> result(v1.size());
    if (subtract)
    {
        for (size_t i = 0; i < v1.size(); ++i)
        {
            result[i] = v1[i] - alpha * v2[i];
        }
    }
    else
    {
        for (size_t i = 0; i < v1.size(); ++i)
        {
            result[i] = v1[i] + alpha * v2[i];
        }
    }
    return result;
}

inline __attribute__((always_inline)) vector<float>
vec_add_mult_FP64(const vector<float> &v1, const vector<float> &v2, float alpha, bool subtract = false)
{
    vector<float> result(v1.size());
    if (subtract)
    {
        for (size_t i = 0; i < v1.size(); ++i)
        {
            result[i] = static_cast<double>(v1[i]) - static_cast<double>(alpha) * static_cast<double>(v2[i]);
        }
    }
    else
    {
        for (size_t i = 0; i < v1.size(); ++i)
        {
            result[i] = static_cast<double>(v1[i]) + static_cast<double>(alpha) * static_cast<double>(v2[i]);
        }
    }
    return result;
}

inline __attribute__((always_inline)) vector<double>
vec_add_mult_FP64(const vector<double> &v1, const vector<float> &v2, double alpha, bool subtract = false)
{
    vector<double> result(v1.size());
    if (subtract)
    {
        for (size_t i = 0; i < v1.size(); ++i)
        {
            result[i] = v1[i] - alpha * static_cast<double>(v2[i]);
        }
    }
    else
    {
        for (size_t i = 0; i < v1.size(); ++i)
        {
            result[i] = v1[i] + alpha * static_cast<double>(v2[i]);
        }
    }
    return result;
}

// Vector Scaling and Addition (r + beta * p)
inline __attribute__((always_inline))
vector<float>
vec_add_scaled(const vector<float> &r, const vector<float> &p, float beta)
{
    vector<float> result(r.size());
    for (size_t i = 0; i < r.size(); ++i)
    {
        result[i] = r[i] + beta * p[i];
    }
    return result;
}

inline __attribute__((always_inline))
vector<float>
vec_add_scaled_FP64(const vector<float> &r, const vector<float> &p, float beta)
{
    vector<float> result(r.size());
    for (size_t i = 0; i < r.size(); ++i)
    {
        result[i] = static_cast<double>(r[i]) + static_cast<double>(beta) * static_cast<double>(p[i]);
    }
    return result;
}

// Euclidean Norm (||v||_2)
inline __attribute__((always_inline)) float norm(const vector<float> &v)
{
    return my_sqrt(dot_product(v, v));
}

int loadMatrix(std::vector<float> &A, std::vector<float> &b)
{
    const char *s = getenv("CG_MATRIX");
    if (s == NULL)
    {
        std::cerr << "CG_MATRIX var not found" << std::endl;
        exit(-1);
    }

    FILE *file = fopen(s, "r");
    if (file == NULL)
    {
        std::cerr << "Error opening matrix file" << std::endl;
        exit(-1);
    }

    // std::vector<std::vector<float>> temp_matrix;
    char line_buf[300000];
    int cols = -1;
    while (fgets(line_buf, sizeof(line_buf), file))
    {
        std::vector<float> row;
        char *token = strtok(line_buf, ",");
        while (token)
        {
            row.push_back(atof(token));
            token = strtok(NULL, ",");
        }
        if (cols == -1)
            cols = row.size();
        else if (row.size() != cols)
        {
            std::cerr << "Non-rectangular matrix detected" << std::endl;
            fclose(file);
            exit(-1);
        }
        // Push the content of row into A
        for (size_t i = 0; i < row.size(); ++i)
        {
            A.push_back(row[i]);
        }
    }
    fclose(file);

    int n = cols;
    A.resize(n * n);
    b.resize(n);
    float sum = 0.0;
    for (int i = 0; i < n; ++i)
    {
        b[i] = 1.0;
    }

    return cols;
}

// --- Conjugate Gradient Algorithm ---
/**
 * Solves the linear system Ax = b using the Conjugate Gradient method.
 * @param A The n x n symmetric positive definite matrix (flattened 1D vector).
 * @param b The right-hand side vector.
 * @param x The initial guess vector (will hold the solution).
 * @param max_iter The maximum number of iterations.
 * @param tolerance The convergence tolerance.
 */
__attribute__((noinline)) __attribute__((annotate("_FPC_CALCULATE_ERROR_")))
vector<double>
conjugate_gradient(
    const vector<float> &A,
    const vector<float> &b,
    size_t max_iter = 1000,
    double tolerance = 1e-6f)
{
    size_t n = b.size();
    vector<double> x(n, 0.0); // Start with an initial guess of 0
    vector<float> x_(n, 0.0f);

    // Initial Residual: r0 = b - A * x0
    vector<float> Ax0 = mat_vec_mult_FP64(A, x_, n);
    vector<float> r = vec_add_mult_FP64(b, Ax0, 1.0f, true); // b - 1.0 * Ax0

    // Initial Search Direction: p0 = r0
    vector<float> p = r;

    float rs_old = dot_product_FP64(r, r); // r^T * r

    // Check initial residual norm
    if (my_sqrt(rs_old) < tolerance)
    {
        return x;
    }

    double relative_b = dot_product_FP64(b, b);
    relative_b = my_sqrt(relative_b);
    cout << "Initial Residual Norm: " << my_sqrt(rs_old) << ", ||b||: " << relative_b << endl;

    // --- Timing variables for average iteration time ---
    std::chrono::duration<double> total_iter_time(0.0);

    // CG Iteration Loop
    for (size_t k = 0; k < max_iter; ++k)
    {
        auto iter_start = std::chrono::high_resolution_clock::now();

        // Compute A * p_k
        vector<float> Ap = mat_vec_mult(A, p, n);

        // Step size: alpha_k = (r_k^T * r_k) / (p_k^T * A * p_k)
        double alpha = static_cast<double>(rs_old) / dot_product_FP64(p, Ap);

        // Update solution: x_{k+1} = x_k + alpha_k * p_k
        x = vec_add_mult_FP64(x, p, alpha);

        // Update residual: r_{k+1} = r_k - alpha_k * (A * p_k)
        r = vec_add_mult_FP64(r, Ap, alpha, true); // r - alpha * Ap

        // Check for convergence
        double rs_new = dot_product_FP64(r, r); // r_{k+1}^T * r_{k+1}
        double relative_residual = my_sqrt(rs_new) / relative_b;
        if (relative_residual < tolerance)
        {
            cout << "Converged in " << k + 1 << " iterations. Residual Norm: " << my_sqrt(rs_new) << endl;
            cout << "Average time per iteration: " << (total_iter_time.count() / (k + 1)) << " seconds" << endl;
            return x;
        }

        // New beta parameter: beta_k = (r_{k+1}^T * r_{k+1}) / (r_k^T * r_k)
        float beta = static_cast<double>(rs_new) / static_cast<double>(rs_old);

        // Update search direction: p_{k+1} = r_{k+1} + beta_k * p_k
        p = vec_add_scaled_FP64(r, p, beta);

        // Prepare for next iteration
        rs_old = rs_new;

        auto iter_end = std::chrono::high_resolution_clock::now();
        total_iter_time += iter_end - iter_start;
    }

    cout << "CG did not converge within " << max_iter << " iterations. Residual Norm: " << my_sqrt(rs_old) << endl;
    return x;
}

// --- Example Usage ---

void print_vector(const string &name, const vector<float> &v)
{
    cout << name << " = [";
    for (size_t i = 0; i < v.size(); ++i)
    {
        cout << v[i] << (i == v.size() - 1 ? "" : ", ");
    }
    cout << "]" << endl;
}

int main(int argc, char **argv)
{
    // Parse command line arguments for max iterations and tolerance
    if (argc < 2)
    {
        cout << "Usage: " << argv[0] << " <max_iter> [tolerance]" << endl;
        return 1;
    }

    size_t max_iter = static_cast<size_t>(atoi(argv[1]));
    double tolerance = (argc > 2) ? atof(argv[2]) : 1e-6;
    cout << "Max Iterations: " << max_iter << ", Tolerance: " << tolerance << endl;

    std::vector<float> A, b;
    int matrix_size = loadMatrix(A, b);
    // Print A as a matrix
    // cout << "Matrix A:" << endl;
    // for (int i = 0; i < matrix_size; ++i)
    //{
    //    for (int j = 0; j < matrix_size; ++j)
    //    {
    //        cout << scientific << setprecision(6) << A[i * matrix_size + j] << " ";
    //    }
    //    cout << endl;
    //}
    // Print b as a vector
    // print_vector("b", b);
    cout << scientific << setprecision(6);

    cout << "Solving Ax = b using Conjugate Gradient..." << endl;
    // print_vector("b", b);

    // Run the CG algorithm
    start_timer();
    vector<double> x_solution = conjugate_gradient(A, b, max_iter, tolerance);
    stop_timer();

    // cout << "\nCG Solution (x):" << endl;
    // print_vector("x", x_solution);

    // Verify the solution (A*x - b should be close to zero)
    vector<float> Ax = mat_vec_mult(A, x_solution, matrix_size);
    vector<float> residual = vec_add_mult(b, Ax, 1.0f, true);

    cout << "\nFinal Residual Norm (||Ax - b||): " << norm(residual) << endl;

    return 0;
}