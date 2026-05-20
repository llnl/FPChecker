#include <iostream>
#include "compute.h"
using namespace std;

float difference_of_squares(float x, float y) {
     return x * x - y * y;
}

// float stable_difference_of_square(float x, float y){
//     float a  = x + y;
//     float b = x - y;
//     return a * b;
//     // return (x + y) * (x - y);
// }