// #include<iostream>

// int main(){
//     float a =100.99, b=1.001, c=2e-9;
//     float result = a + b + c;
//     float buffer[6];

//     buffer[1] = result;
//     buffer[2] = result;
//     buffer[5] = result;
//     return 0;
// }



// void store_result(float *a, float *b,  float x, float y) {
//     float result = x + y;
//     *a = result;
//     *b = result;
   
// }

// int main() {
//     float v1, v2;
//     store_result(&v1, &v2, 1.0f, 2.0f);
//     printf("%f %f %f\n", v1, v2);
//     return 0;
// }


// void multiple_stores(float a, float b, float *ptr1, float *ptr2, float *ptr3) {
//     float result = a + b;
//     *ptr1 = result;
//     *ptr2 = result;
//     *ptr3 = result;
// }

// int main() {
//     float a = 3.5f;
//     float b = 2.5f;
//     float x, y, z;
    
   
//     multiple_stores(a, b, &x, &y, &z);
    
//     // // Alternative: direct multiple stores in main
//     // float result = a + b;
//     // float location1 = result;
//     // float location2 = result;
//     // float location3 = result;
    
//     return 0;
// }

// checking the square 
int main(){
    float x= 1e-2f, result = 0.0f;
    result = x * x;
    return 0;
}

// // Cross function with arrays
// void compute_array(float* arr, int size) {
//     for (int i = 0; i < size; i++) {
//         arr[i] = arr[i] + 5.001f;  
//     }
// }

// int main() {
//     float data[5] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
//     compute_array(data, 5);  // Errors propagate through memory addresses
//     return 0;
// }

// Complex Expression with Spilling
// float complex_calc(float a, float b, float c, float d, float e, float f) {
//     float temp1 = a + b;  // Might be spilled
//     float temp2 = c + d;  // Might be spilled  
//     float temp3 = e + f;  // Might be spilled
//     return temp1 + temp2 + temp3;  // Loads from spilled locations
// }