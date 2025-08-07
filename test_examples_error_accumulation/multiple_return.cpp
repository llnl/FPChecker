#include <iostream>
using namespace std;

// typedef float Real_t;

FPC_CALCULATE_ERROR
void sum_and_diff(float a, float b, float *sum, float *diff) {
    printf("...in sum\n");
    *sum  = (a + b)+2.000034f - (a/2.0f);
    *diff = a - b;
}

// FPC_CALCULATE_ERROR
int main(int argc, char **argv) {

    float a = atof(argv[1]);
    float b = atof(argv[2]);
    float s, d;

    sum_and_diff(a, b, &s, &d);

    printf("Sum = %f, Difference = %f\n", s, d);
    return 0;
}
