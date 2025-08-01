#include <iostream>
using namespace std;

typedef float Real_t;
//typedef double Real_t;

// float fused_operation(float a, float b, float c) {
//     return a * b + c;
// }

// int main(int argc, char** argv) {

//     float a = atof(argv[1]);
//     float b = atof(argv[2]);
//     float c = atof(argv[3]);

//     float result = fused_operation(a, b, c);
//     result = result * b + c;
//     b = a + b ;
//     result = result * 2+ 3;
//     printf("FMA result: %f\n", result);
//     return 0;
// }

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

// int main(int argc, char** argv) {
//     float a = atof(argv[1]);
//     float b = atof(argv[2]);
//     float c = atof(argv[3]);

//     float result = a * b + c;
//     //result = result + 10.0000f;
//     cout << "FMA result: "<< result <<endl;
//     return 0;
// }

// int main(int argc, char** argv) {
//     float a = atof(argv[1]);
//     float b = atof(argv[2]);
//     float c = atof(argv[3]);
//     float sum = a + 10.0f ;
//     float sum2 = b + 20 ;
//     printf("result %f\n", sum2);
//     return 0;
// }

// int main(int argc, char** argv){
//     float a = atof(argv[1]);
//     float b = atof(argv[2]);
//     a = a + 1.0f;
//     b = b + 2.0f;
//     float sum = a + b;
//     printf("sum %f\n", sum);
// }