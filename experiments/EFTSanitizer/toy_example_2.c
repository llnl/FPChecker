#include <stdio.h>

float derivates(float x) {
    return x * x;
}

double derivates_double(double x) {
    return x * x;
}

float finite_difference(float x, float h) {
    return (derivates(x + h) - derivates(x)) / h;
}

double finite_difference_double(double x, double h) {
    return (derivates_double(x + h) - derivates_double(x)) / h;
}

int main(void) {
    float x = 200.0f;
    float h = 0.0002f;
    float fd_result = finite_difference(x, h);
    fd_result  = fd_result + 0 ;
    double fd_result_double = finite_difference_double((double)x, (double)h);

    printf("Finite difference result (float): %.8f\n", fd_result);
    printf("Finite difference result (double): %.17f\n", fd_result_double);

    double error = fd_result_double - (double)fd_result;
    printf("Error: %.17f\n", error);

    return 0;
}