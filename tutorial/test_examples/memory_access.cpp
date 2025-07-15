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