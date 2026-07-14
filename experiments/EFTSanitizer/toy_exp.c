#include <stdio.h>

int main(void) {
    // volatile double a = 9007199254740992.0;  // 2^53
    volatile double a = 100.0; 
    volatile double b = 0.1;

    double s = a + b;        // line A: exact should be 2^53 + 1, computed is 2^53
    double c = s - a;        // line B: exact should be 1, computed is 0

    double exact = 0.1;
    double actual_error = exact - c;
    actual_error = actual_error + 0;
    
    printf( "s = %.17e\n", s);
    printf("c = %.17e\n", c);
    printf("Actual error: %.17e\n", actual_error);

    return 0;
}



// int main(void) {
//     // volatile double a = 9007199254740992.0;  // 2^53
//     volatile float a = 100.0; 
//     volatile float b = 0.1;

//     float s = a + b;        // line A: exact should be 2^53 + 1, computed is 2^53
//     float c = s - a;        // line B: exact should be 1, computed is 0

//     float exact = 0.1;
//     float actual_error = exact - c;

//     printf( "s = %.17e\n", s);
//     printf("c = %.17e\n", c);
//     printf("Actual error: %.17e\n", actual_error);

//     return 0;
// }
