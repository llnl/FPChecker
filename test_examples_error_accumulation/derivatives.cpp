#include <iostream>
using namespace std;

typedef float Real_t;
//typedef double Real_t;


FPC_CALCULATE_ERROR
int main(int argc, char **argv){
    
    Real_t x = atof(argv[1]);
    Real_t epsilon = atof(argv[2]);

    Real_t fx = x * x;                                    // f(x) = x^2
    Real_t x_plus_eps = x + epsilon;                      // x + epsilon
    Real_t fx_with_eps = x_plus_eps * x_plus_eps;        // f(x + eps) = (x + eps)^2
    
    Real_t derivative = (fx_with_eps - fx) / epsilon;
    cout << "derivative: " << derivative << endl;
    // double test = finite_difference_double(x, epsilon);
    return 0;

}