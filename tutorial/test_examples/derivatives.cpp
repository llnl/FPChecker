#include <iostream>
using namespace std;

float derivates (float x) {
    return x * x;
}

float finite_difference( float x, float eps){
    float fx_with_eps = derivates(x + eps);
    float fx = derivates(x);

    float derivative = (fx_with_eps - fx) / eps;

    cout << "f(x + eps): " << fx_with_eps << endl;
    cout << "fx: " << fx <<endl;
    cout << "derivative: " << derivative << endl;

    return derivative;

}



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
    
    float x = atof(argv[1]);
    float epsilon = atof(argv[2]);
    float float_result = finite_difference(x, epsilon);

    // double x_double = (double)x;
    // double eps_double = (double)epsilon;
    // double double_result = finite_difference_double(x_double, eps_double);
    // cout << "Difference " << (double_result - float_result) << endl;

    return 0;

}