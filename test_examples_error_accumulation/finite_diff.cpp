#include <iostream>
using namespace std;

#include "square.cpp"
#include "square_shifted.cpp"

typedef float Real_t;

// float shifted_square(float x, float h)
// {
//     Real_t x_plus_eps = x + h;
//     Real_t fx_x_h = x_plus_eps * x_plus_eps;
//     return fx_x_h;
// }
FPC_CALCULATE_ERROR

int main(int argc, char **argv){

    Real_t x = atof(argv[1]);
    Real_t epsilon = atof(argv[2]);

    Real_t sq = fsquare(x) ;                              
    Real_t res = shifted_square (x , epsilon)  ;                   
        
    Real_t outcome = (res - sq ) / epsilon;
    // cout << "Multi-file derivative result:" << outcome <<endl;
    printf("Multiple file outcome: %f\n", outcome);
    return 0;

}