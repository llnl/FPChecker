#include<iostream>
using namespace  std;


int main(int argc, char** argv) {
    float value = atof(argv[1]);
    int times = atoi(argv[2]);
    float sum = 0.0f;
    for (int i = 0; i < times; ++i) {
        sum += value;
    }
    printf("Sum: %.10f\n", sum);
    return 0;
}