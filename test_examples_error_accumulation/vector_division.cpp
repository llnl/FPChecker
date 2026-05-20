#include<iostream>
using namespace std;


float vector_division(float *x, float *y, size_t s) {
    float result;
    for (int i=0; i < s; ++i) {
        float tmp = x[i] / y[i];
        result += tmp;
    }
    return result;
}

int main(int argc, char **argv) {
    
    // int size = atoi(argv[argc - 1]); 
    // float x[size], y[size];          

    // for (int i = 0; i < size; ++i) {
    //     x[i] = atof(argv[1 + i]);         
    //     y[i] = atof(argv[1 + size + i]);  
    // }
    float x[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float y[] = {0.5f, 1.0f, 1.5f, 2.0f};
    int size = 4;

    float result = vector_division(x, y, size); 
    printf("Result = %.12f\n", result); 

    return 0;
}