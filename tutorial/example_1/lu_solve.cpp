#include <iostream>
#include <vector>
#include <string>

#include "../common/linear_solvers.hpp"
#include "../common/blas.hpp"
#include "../common/io.hpp"

using namespace std;

int main(int argc, char *argv[])
{
    // Check if the correct number of arguments is provided
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <matrix file>" << std::endl;
        return 1;
    }

    string filename = "./matrix.csv";
    filename = argv[1];

    cout << "Loading matrix A:" << endl;
    vector<vector<double>> A = load_matrix_from_csv(filename);
    print_matrix(A);
    cout << "-----------------------------------------------------------" << endl;

    // Solve linear system Ax = 1
    vector<double> b(A.size(), 1.0);
    auto x = solve_system_with_LU(A, b);

    // Print solution
    cout << "Solution x:" << endl;
    for (size_t i = 0; i < x.size(); ++i)
        cout << "x[" << i << "]: " << x[i] << endl;
    cout << "-----------------------------------------------------------" << endl;

    // Residual
    auto a = multiply_matrix_vector(A, x);  // calculate Ax
    auto residual = subtract_vectors(a, b); // calculate Ax - b
    auto norm = l2_norm(residual);          // calculate ||Ax - b||
    cout << "Residual norm ||Ax - b||: " << norm << endl;

    
    return 0;
}