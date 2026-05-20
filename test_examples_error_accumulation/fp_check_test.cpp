#include<iostream>
using namespace std;



// double comp_interest(double principal, double rate, int years){
//     double percentage = rate/100.0;
//     double amount = principal;
//     double prev_amount = principal;

//     for (int i = 1; i<=years; ++i){
//         double interest = amount * percentage;
//         amount = amount + interest;
//         double change = amount - prev_amount ;
//         cout <<"Year "<< i << ": " <<amount << " | Change: " << change <<endl;
//         prev_amount = amount;
//     }

//     return amount;

// }

// int main(int argc, char **argv){
//     double principal = atof(argv[1]);
//     double rate = atof(argv[2]);
//     int year = atoi(argv[3]);

//     comp_interest(principal, rate, year);
//     return 0;

// }

#include <iostream>
using namespace std;

FPC_CALCULATE_ERROR

void test_error_propagation(float x, float y, float u, float v) {
    // These should create different errors
    float a = x - y;    // Catastrophic cancellation -> BIG error (Register %1)
    float b = u * v;    // Simple multiplication -> small/no error (Register %2)
    
    // Allocate memory locations
    float result1, result2;
    float *ptr1 = &result1;
    float *ptr2 = &result2;
    
    *ptr1 = a;          
    *ptr2 = b; 
    printf("a = %.7f, b = %.7f\n", a, b);        
}

int main(int argc, char *argv[]) {

    float x = atof(argv[1]);
    float y = atof(argv[2]);
    float u = atof(argv[3]);
    float v = atof(argv[4]);
    
    test_error_propagation(x, y, u, v);
    return 0;
}

#include <cstdio>

// A function that does some computation and returns two results
void compute_all(float x, float y, float z, float* out_sum, float* out_expr) {
    float sum = x + y + z;
    float expr = (x + 2.0f) * (y - 0.5f) - z;
    *out_sum = sum;
    *out_expr = expr;
}

int main() {
    float x = 1.5f, y = 2.5f, z = 3.5f;
    float result_sum, result_expr;

    compute_all(x, y, z, &result_sum, &result_expr);

    printf("Sum: %.5f\n", result_sum);
    printf("Expr: %.5f\n", result_expr);

    // (your instrumentation will auto-generate the dataflow tracking)
    return 0;
}
