#include<iostream>
using namespace  std;

typedef float my_precision;
//typedef double my_precision;


int main(int argc, char** argv) {
    my_precision value = atof(argv[1]);
    int times = atoi(argv[2]);
    //int times  = 3;
    my_precision sum = 0.0f;
    for (int i = 0; i < times; ++i) {
        sum += value;
    }
    printf("Sum: %.7f\n", sum);
    return 0;
}