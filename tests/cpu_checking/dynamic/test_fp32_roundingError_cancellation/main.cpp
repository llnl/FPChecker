#include <iostream>
#include "compute.h"
using namespace std;


int main(int argc, char **argv){
    
    // float x = 2e2; //1000000.0f;     
    // float epsilon = 2.02e2  //1e-7f; 
    float x = atof(argv[1]);
    float y = atof(argv[2]);
     float diff_of_square = difference_of_squares(x, y);
    
    //  float stable_diff_of_square = stable_difference_of_square(x , y);
    // printf("Stable diff of square:",stable_diff_of_square);

    return 0;

}