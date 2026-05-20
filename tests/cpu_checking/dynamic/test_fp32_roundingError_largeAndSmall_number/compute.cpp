#include "compute.h"
#include <vector>
#include <iostream>
using namespace std;


// float compute_vector_sum(vector<float>& vec) {
//     float sum = 0.0;
//     for (float value : vec) {
//         sum += value;
//     }
//     cout << "Sum:" << sum  <<endl;

//     return sum;
// }

float finance_computing(float basic, float interest, int year) {
    float total = 0.0f;
    for (int i = 1; i <= year; i++) {
        float gain = basic * interest * i;
        total += gain;
    }
    return total;
}