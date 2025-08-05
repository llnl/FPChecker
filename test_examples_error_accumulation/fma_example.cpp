#include <iostream>
using namespace std;

typedef float Real_t;
//typedef double Real_t;

FPC_CALCULATE_ERROR
int main(int argc, char** argv) {
    if (argc < 4) {
        printf("Usage: %s a b c\n", argv[0]);
        return 1;
    }

    Real_t a = atof(argv[1]);
    Real_t b = atof(argv[2]);
    Real_t c = atof(argv[3]);

    Real_t result = a * b + c;
    result = result * b + c;
    b = a + b;
    result = result * 2 + 3;

    printf("FMA result: %f\n", result);
    return 0;
}
