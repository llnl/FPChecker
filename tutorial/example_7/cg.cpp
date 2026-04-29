#include <iostream>
#include <vector>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <iomanip>
#include <chrono>
#include <cstring>

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

// Euclidean Norm (||v||_2)
inline __attribute__((always_inline)) float norm(const vector<float> &v)
{
    return my_sqrt(dot_product(v, v));
}

int loadMatrix(const char *matrix_path, std::vector<float> &A, std::vector<float> &b)
{
    if (matrix_path == NULL)
    {
        std::cerr << "Matrix path not provided" << std::endl;
        exit(-1);
    }

    FILE *file = fopen(matrix_path, "r");
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
            cout << "Rows: " << row.size() << ", Cols: " << cols << endl;
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
vector<float>
conjugate_gradient(
    const vector<float> &A,
    const vector<float> &b,
    size_t max_iter = 1000,
    float tolerance = 1e-6f)
{
    size_t n = b.size();
    vector<float> x(n, 0.0f); // Start with an initial guess of 0

    // Initial Residual: r0 = b - A * x0
    vector<float> Ax0 = mat_vec_mult(A, x, n);
    vector<float> r = vec_add_mult(b, Ax0, 1.0f, true); // b - 1.0 * Ax0

    // Initial Search Direction: p0 = r0
    vector<float> p = r;

    float rs_old = dot_product(r, r); // r^T * r

    // Check initial residual norm
    if (my_sqrt(rs_old) < tolerance)
    {
        return x;
    }

    float relative_b = dot_product(b, b);
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
        float alpha = rs_old / dot_product(p, Ap);

        // Update solution: x_{k+1} = x_k + alpha_k * p_k
        x = vec_add_mult(x, p, alpha);

        // Update residual: r_{k+1} = r_k - alpha_k * (A * p_k)
        r = vec_add_mult(r, Ap, alpha, true); // r - alpha * Ap

        // Check for convergence
        float rs_new = dot_product(r, r); // r_{k+1}^T * r_{k+1}
        float relative_residual = my_sqrt(rs_new) / relative_b;
        if (relative_residual < tolerance)
        {
            cout << "Converged in " << k + 1 << " iterations. Residual Norm: " << my_sqrt(rs_new) << endl;
            cout << "Average time per iteration: " << (total_iter_time.count() / (k + 1)) << " seconds" << endl;
            return x;
        }

        // New beta parameter: beta_k = (r_{k+1}^T * r_{k+1}) / (r_k^T * r_k)
        float beta = rs_new / rs_old;

        // Update search direction: p_{k+1} = r_{k+1} + beta_k * p_k
        p = vec_add_scaled(r, p, beta);

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
    if (argc < 3)
    {
        cout << "Usage: " << argv[0] << " <matrix.csv> <max_iter> [tolerance]" << endl;
        return 1;
    }

    const char *matrix_path = argv[1];
    size_t max_iter = static_cast<size_t>(atoi(argv[2]));
    float tolerance = (argc > 3) ? atof(argv[3]) : 1e-6f;
    cout << "Max Iterations: " << max_iter << ", Tolerance: " << tolerance << endl;

    std::vector<float> A, b;
    int matrix_size = loadMatrix(matrix_path, A, b);
    cout << scientific << setprecision(6);

    cout << "Solving Ax = b using Conjugate Gradient..." << endl;

    // Run the CG algorithm
    start_timer();
    vector<float> x_solution = conjugate_gradient(A, b, max_iter, tolerance);
    stop_timer();

    // Verify the solution (A*x - b should be close to zero)
    vector<float> Ax = mat_vec_mult(A, x_solution, matrix_size);
    vector<float> residual = vec_add_mult(b, Ax, 1.0f, true);

    cout << "\nFinal Residual Norm (||Ax - b||): " << norm(residual) << endl;

    return 0;
}