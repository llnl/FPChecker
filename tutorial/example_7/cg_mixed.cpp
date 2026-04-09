// cg_mixed.cpp — Mixed-precision CG guided by FPChecker rounding-error report
//
// Storage: matrix A, vectors r, p, Ap, b stay float (FP32).
// Arithmetic: dot products, scalar ratios (alpha, beta), and vector
//             updates use FP64 intermediates where the report showed
//             high relative error.  Solution vector x is kept in FP64
//             because it accumulates corrections over many iterations.
//
// Report-driven decisions (line numbers refer to cg.cpp, κ≈4e4 matrix):
//
//   HIGH ERROR — promoted to FP64 intermediates:
//     Line  52: result += v1[i]*v2[i]  (dot_product, rel err 1.0)
//     Line 230: alpha = rs_old / dot(p,Ap)  (rel err 2.4e5 — highest!)
//     Line 249: beta = rs_new / rs_old  (rel err 0.33)
//     Line  90: result[i] = v1[i] - alpha*v2[i]  (residual update, rel err 1.0)
//     Line  97: result[i] = v1[i] + alpha*v2[i]  (solution update, rel err 0.83)
//     Line 111: result[i] = r[i] + beta*p[i]  (direction update, rel err 1.0)
//     Line  75: result[i] += A[i*n+j]*v[j]  (SpMV accumulation, rel err 1.04)
//     Line  39: guess = 0.5f*(guess + x/guess)  (my_sqrt, rel err 1.0)
//
//   KEPT AS FP32 (low or zero error):
//     Matrix storage, vector storage, loadMatrix, I/O.
//
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

// my_sqrt — promoted to FP64 (report line 39: rel err 1.0)
inline __attribute__((always_inline)) double my_sqrt(double x)
{
    if (x < 0.0)
    {
        std::cout << "sqrt: negative input" << std::endl;
        return -1.0;
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

// Dot Product — FP64 accumulation (report line 52: rel err 1.0)
// Vectors stay float; accumulator is double to reduce rounding.
inline __attribute__((always_inline))
double dot_product(const vector<float> &v1, const vector<float> &v2)
{
    double result = 0.0;
    for (size_t i = 0; i < v1.size(); ++i)
    {
        result += static_cast<double>(v1[i]) * static_cast<double>(v2[i]);
    }
    return result;
}

// Matrix-Vector Multiplication — FP64 row accumulation (report line 75: rel err 1.04)
// Matrix and output stay float; inner product per row computed in double.
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
        double acc = 0.0;
        for (size_t j = 0; j < n; ++j)
        {
            acc += static_cast<double>(A[i * n + j]) * static_cast<double>(v[j]);
        }
        result[i] = static_cast<float>(acc);
    }
    return result;
}

// Vector update (float result) — FP64 intermediate arithmetic
// (report lines 90, 97: rel err 1.0 and 0.83)
inline __attribute__((always_inline))
vector<float>
vec_add_mult(const vector<float> &v1, const vector<float> &v2,
             double alpha, bool subtract = false)
{
    vector<float> result(v1.size());
    if (subtract)
    {
        for (size_t i = 0; i < v1.size(); ++i)
        {
            result[i] = static_cast<float>(
                static_cast<double>(v1[i]) - alpha * static_cast<double>(v2[i]));
        }
    }
    else
    {
        for (size_t i = 0; i < v1.size(); ++i)
        {
            result[i] = static_cast<float>(
                static_cast<double>(v1[i]) + alpha * static_cast<double>(v2[i]));
        }
    }
    return result;
}

// Vector update for solution x (double result) — x accumulates over all
// iterations so it must stay in FP64 to avoid drift.
inline __attribute__((always_inline))
vector<double>
vec_add_mult_d(const vector<double> &v1, const vector<float> &v2,
               double alpha)
{
    vector<double> result(v1.size());
    for (size_t i = 0; i < v1.size(); ++i)
    {
        result[i] = v1[i] + alpha * static_cast<double>(v2[i]);
    }
    return result;
}

// Search direction update — FP64 intermediate (report line 111: rel err 1.0)
inline __attribute__((always_inline))
vector<float>
vec_add_scaled(const vector<float> &r, const vector<float> &p, double beta)
{
    vector<float> result(r.size());
    for (size_t i = 0; i < r.size(); ++i)
    {
        result[i] = static_cast<float>(
            static_cast<double>(r[i]) + beta * static_cast<double>(p[i]));
    }
    return result;
}

// Euclidean Norm
inline __attribute__((always_inline)) double norm(const vector<float> &v)
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
        else if ((int)row.size() != cols)
        {
            cout << "Rows: " << row.size() << ", Cols: " << cols << endl;
            std::cerr << "Non-rectangular matrix detected" << std::endl;
            fclose(file);
            exit(-1);
        }
        for (size_t i = 0; i < row.size(); ++i)
        {
            A.push_back(row[i]);
        }
    }
    fclose(file);

    int n = cols;
    A.resize(n * n);
    b.resize(n);
    for (int i = 0; i < n; ++i)
    {
        b[i] = 1.0;
    }

    return cols;
}

// --- Conjugate Gradient Algorithm (Mixed Precision) ---
//
// Compared to the pure-FP32 version in cg.cpp, the changes are:
//   1. dot_product accumulates in FP64  (fixes report line 52)
//   2. alpha and beta computed as double (fixes report lines 230, 249)
//   3. mat_vec_mult accumulates rows in FP64 (fixes report line 75)
//   4. vec_add_mult uses FP64 intermediates (fixes report lines 90, 97)
//   5. vec_add_scaled uses FP64 intermediates (fixes report line 111)
//   6. Solution vector x stored in FP64 (prevents drift over iterations)
//   7. my_sqrt computed in FP64 (fixes report line 39)
//
__attribute__((noinline))
vector<double>
conjugate_gradient(
    const vector<float> &A,
    const vector<float> &b,
    size_t max_iter = 1000,
    double tolerance = 1e-6)
{
    size_t n = b.size();
    vector<double> x(n, 0.0);   // FP64: accumulates corrections each iteration
    vector<float> x_f(n, 0.0f); // FP32 copy for mat-vec with FP32 matrix

    // Initial Residual: r0 = b - A * x0
    vector<float> Ax0 = mat_vec_mult(A, x_f, n);
    vector<float> r = vec_add_mult(b, Ax0, 1.0, true); // b - 1.0 * Ax0

    // Initial Search Direction: p0 = r0
    vector<float> p = r;

    // FP64 scalar: fixes report line 52 (dot_product rel err 1.0)
    double rs_old = dot_product(r, r); // r^T * r

    // Check initial residual norm
    if (my_sqrt(rs_old) < tolerance)
    {
        return x;
    }

    double relative_b = my_sqrt(dot_product(b, b));
    cout << "Initial Residual Norm: " << my_sqrt(rs_old) << ", ||b||: " << relative_b << endl;

    // --- Timing variables for average iteration time ---
    std::chrono::duration<double> total_iter_time(0.0);

    // CG Iteration Loop
    for (size_t k = 0; k < max_iter; ++k)
    {
        auto iter_start = std::chrono::high_resolution_clock::now();

        // Compute A * p_k  (FP64 row accumulation, FP32 result)
        vector<float> Ap = mat_vec_mult(A, p, n);

        // Step size in FP64: fixes report line 230 (rel err 2.4e5)
        double alpha = rs_old / dot_product(p, Ap);

        // Update solution in FP64: x_{k+1} = x_k + alpha_k * p_k
        x = vec_add_mult_d(x, p, alpha);

        // Update residual (FP64 intermediate, FP32 storage):
        // fixes report line 90 (rel err 1.0)
        r = vec_add_mult(r, Ap, alpha, true); // r - alpha * Ap

        // Check for convergence
        double rs_new = dot_product(r, r);
        double relative_residual = my_sqrt(rs_new) / relative_b;
        if (relative_residual < tolerance)
        {
            cout << "Converged in " << k + 1 << " iterations. Residual Norm: " << my_sqrt(rs_new) << endl;
            cout << "Average time per iteration: " << (total_iter_time.count() / (k + 1)) << " seconds" << endl;
            return x;
        }

        // Beta in FP64: fixes report line 249 (rel err 0.33)
        double beta = rs_new / rs_old;

        // Update search direction (FP64 intermediate, FP32 storage):
        // fixes report line 111 (rel err 1.0)
        p = vec_add_scaled(r, p, beta);

        // Prepare for next iteration
        rs_old = rs_new;

        auto iter_end = std::chrono::high_resolution_clock::now();
        total_iter_time += iter_end - iter_start;
    }

    cout << "CG did not converge within " << max_iter << " iterations. Residual Norm: " << my_sqrt(rs_old) << endl;
    return x;
}

// --- Main ---

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
    cout << scientific << setprecision(6);

    cout << "Solving Ax = b using Conjugate Gradient (mixed precision)..." << endl;

    start_timer();
    vector<double> x_solution = conjugate_gradient(A, b, max_iter, tolerance);
    stop_timer();

    // Verify the solution: compute Ax in FP32 (same as cg.cpp), then ||Ax - b||
    // Convert x_solution to float for the FP32 mat-vec
    vector<float> x_f(matrix_size);
    for (int i = 0; i < matrix_size; ++i)
        x_f[i] = static_cast<float>(x_solution[i]);

    vector<float> Ax = mat_vec_mult(A, x_f, matrix_size);
    vector<float> residual = vec_add_mult(b, Ax, 1.0, true);

    cout << "\nFinal Residual Norm (||Ax - b||): " << norm(residual) << endl;

    return 0;
}