#include <iostream>
using namespace std;

FPC_CALCULATE_ERROR
void complex_calculation(float a, float b, float c, float d, float *numerator, float *denominator) {
    *numerator = a * b + c * d;      
    *denominator = a + b - c + d;     
}

int main() {
    float a = 2.0f, b = 3.0f, c = 0.1f, d = 1.5f;
    float num, denom;
    
    complex_calculation(a, b, c, d, &num, &denom);
    
    printf("Numerator: %.6f\n", num);
    printf("Denominator: %.6f\n", denom);
    return 0;
}