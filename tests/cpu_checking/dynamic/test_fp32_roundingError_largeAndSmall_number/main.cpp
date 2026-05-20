#include <iostream>
#include <vector>
#include "compute.h"
using namespace std;


int main() {

    // vector<float> vec = {1.0f, 2.0f, 3.0f, 1e8f};
    // float sum_of_vector = compute_vector_sum(vec);
    
    float basic = 1e8;  
    float interest = 0.0002;    
    int year = 3;            
    
    float sum = finance_computing(basic, interest, year);
    
    
    return 0;
}