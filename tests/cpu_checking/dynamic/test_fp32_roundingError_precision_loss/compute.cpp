#include <iostream>
#include "compute.h"
using namespace std;

float derivates(float x) {
    return x * x;
}

float finite_difference(float x, float eps) {
    float fx_with_eps = derivates(x + eps);
    float fx = derivates(x);

    float diff = (fx_with_eps - fx) ;
    float derivative = diff / eps;  
    
    cout << "f(x + eps): " << fx_with_eps << endl;
    cout << "fx: " << fx << endl;
    cout << "derivative: " << derivative << endl;
    
    return derivative;
}