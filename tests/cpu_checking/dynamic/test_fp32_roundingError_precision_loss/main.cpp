#include <iostream>
#include "compute.h"
using namespace std;


int main(int argc, char **argv){
    
    // float x = 2e2; //1000000.0f;     
    // float epsilon = 0.0002;  //1e-7f; 
    float x = atof(argv[1]);
    float epsilon = atof(argv[2]);
    
    float float_result = finite_difference(x, epsilon);
    printf("final result: %.9f\n", float_result);

    return 0;
}