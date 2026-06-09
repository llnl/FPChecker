#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cfloat>
#include <numeric>

using namespace std;

inline __attribute__((always_inline)) double float_abs(double x)
{
    return x < 0.0 ? -x : x;
}

FPC_CALCULATE_ERROR
tuple<vector<vector<double>>, vector<vector<double>>, vector<vector<double>>>
lu_factorization_partial_pivot(vector<vector<double>> A)
{
    // Check matrix is square
    int m = A.size();
    int n = A[0].size();
    if (m != n)
    {
        cerr << "Error: Input matrix must be square for LU factorization." << endl;
        return make_tuple(vector<vector<double>>(), vector<vector<double>>(), vector<vector<double>>());
    }

    vector<vector<double>> U = A;
    vector<vector<double>> L(m, vector<double>(m, 0.0));
    vector<vector<double>> P(m, vector<double>(m, 0.0));

    // Initialize L and P as identity matrices
    for (int i = 0; i < m; ++i)
    {
        L[i][i] = 1.0;
        P[i][i] = 1.0;
    }

    for (int k = 0; k < m - 1; ++k)
    {
        // Find the row with the maximum absolute value in the k-th column (from row k downwards)
        int pivot_row = k;
        for (int i = k + 1; i < m; ++i)
        {
            if (float_abs(U[i][k]) > float_abs(U[pivot_row][k]))
            {
                pivot_row = i;
            }
        }

        // Swap rows if a better pivot is found
        if (pivot_row != k)
        {
            if (k <= m)
                swap(U[k], U[pivot_row]);
            if ((k - 1) >= 0)
            {
                // swap(L[k], L[pivot_row]);
                for (int j = 0; j <= (k - 1); ++j)
                {
                    swap(L[k][j], L[pivot_row][j]);
                }
            }

            swap(P[k], P[pivot_row]);
        }

        // Perform elimination
        for (int j = k + 1; j < m; ++j)
        {
            L[j][k] = U[j][k] / U[k][k];
            for (int l = k; l < m; ++l)
            {
                U[j][l] = U[j][l] - L[j][k] * U[k][l];
                if (float_abs(U[j][l]) < (10 * DBL_EPSILON))
                    U[j][l] = 0.0; // Set value to zero if close to machine epsilon
            }
        }
    }

    return make_tuple(L, U, P);
}

int main(int argc, char *argv[])
{
    // Example usage
    vector<vector<double>> A = {
        {4, 3, 2},
        {2, 1, 3},
        {1, 2, 4}};

    auto [L, U, P] = lu_factorization_partial_pivot(A);

    // Print the results with tabs between elements
    cout << "L matrix:" << endl;
    for (const auto &row : L)
    {
        for (const auto &val : row)
        {
            cout << val << "\t";
        }
        cout << endl;
    }
    cout << endl;

    cout << "U matrix:" << endl;
    for (const auto &row : U)
    {
        for (const auto &val : row)
        {
            cout << val << "\t";
        }
        cout << endl;
    }
    cout << endl;

    cout << "P matrix:" << endl;
    for (const auto &row : P)
    {
        for (const auto &val : row)
        {
            cout << val << "\t";
        }
        cout << endl;
    }

    return 0;
}
