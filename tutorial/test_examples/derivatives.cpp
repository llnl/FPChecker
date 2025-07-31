#include <iostream>
using namespace std;

typedef float Real_t;
//typedef double Real_t;

// Real_t derivates (Real_t x) {
//     return x * x;
// }

// Real_t finite_difference( Real_t x, Real_t eps){
//     Real_t fx_with_eps = derivates(x + eps);
//     Real_t fx = derivates(x);

//     Real_t derivative = (fx_with_eps - fx) / eps;
//     cout << "derivative: " << derivative << endl;
//     return derivative;

// }


// double derivatives_double(double x){
//     return x * x;
// }

// double finite_difference_double(double x, double eps) {
//     double fx_with_eps = derivatives_double(x + eps);
//     double fx = derivatives_double(x);
//     double derivative = (fx_with_eps - fx) / eps;
//     cout << "derivative in double: " << derivative << endl;
//     return derivative;
// }

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