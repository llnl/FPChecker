#include<iostream>
using namespace  std;

void load_compute_store(float* arr) {
    float tmp= arr[0] / arr[1];
    arr[2] = tmp;
    float tmp2 = arr[2];
    float tmp3 = tmp2 * 3.14f;
    arr[3] = tmp3;  
    
}

int main(){
    float array[5] ={2.0f, 3.0f, 1.0f, 5.0f, 4.0f};
    load_compute_store(array);
    printf("Comp: %f\n", array[3]);
    
    return 0;
}


//program 1
// #include <stdio.h>

// int main() {
//     float a = 1.5f, b = 2.5f;
//     float result = a + b;  // FP operation

//     float x, y;
//     x = result;  // Store 1
//     y = result;  // Store 2

//     printf("x = %.2f, y = %.2f\n", x, y);
//     return 0;
// }

//Program 2 
// #include <stdio.h>

// int main() {
//     float a = 3.0f, b = 4.0f, d = 2.0f;

//     float tmp = a + b;  // FP operation
//     float c;
//     c = tmp;            // Store the result

//     float reused = c;   // Load the stored value
//     float final = reused * d;  // Use it in another FP operation

//     printf("final = %.2f\n", final);
//     return 0;
// }