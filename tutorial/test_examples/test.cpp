#include<iostream>

// int main(){
//     float a =100.99, b=1.001, c=2e-9;
//     float result = a + b + c;
//     float buffer[6];

//     buffer[1] = result;
//     buffer[2] = result;
//     buffer[5] = result;
//     return 0;
// }


// Cross function with arrays
void compute_array(float* arr, int size) {
    for (int i = 0; i < size; i++) {
        arr[i] = arr[i] + 5.001f;  
    }
}

int main() {
    float data[5] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    compute_array(data, 5);  // Errors propagate through memory addresses
    return 0;
}

// Complex Expression with Spilling
// float complex_calc(float a, float b, float c, float d, float e, float f) {
//     float temp1 = a + b;  // Might be spilled
//     float temp2 = c + d;  // Might be spilled  
//     float temp3 = e + f;  // Might be spilled
//     return temp1 + temp2 + temp3;  // Loads from spilled locations
// }