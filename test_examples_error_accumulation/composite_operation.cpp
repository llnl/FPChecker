#include<iostream>
using namespace std;

FPC_CALCULATE_ERROR
float composite_operation(float x, float y , float z) {
    float result = (x + 1.0f) * (y + 2.0002f) - x * x - 2.0f * z - 1.0f;  
    return result;  // Should be 0, but floating point errors accumulate
}

int main(){
    float a = 2.0f, b = 3.0f, c = 0.1f;
    printf("Result : %.7f\n", composite_operation(a, b, c));
    return 0;
}


