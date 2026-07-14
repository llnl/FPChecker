#include <stdio.h>
#include <math.h>
extern void eftsan_print_error(double);
// From Kahan "A survey of error analysis"
int main( int argc, const char* argv[] )
{
    float fa = 7169.0;
    float fb = -8686.0;
    float fc = 2631.0;
    
    float fbb = fb * fb;
    float f4ac = 4.0 * fa * fc;
    float sub = fbb - f4ac;
    float fsqt = sqrtf(sub);
    float numer1 = -1.0 * fb + fsqt;
    float root1 = numer1 / (2.0 * fa);
    float numer2 = -1.0 * fb - fsqt;
    float root2 = numer2 / (2.0 * fa);
    printf("%.50e\n", root1);
    printf("%.50e\n", root2);
    eftsan_print_error(root1);
    eftsan_print_error(root2);
    return 0;
}


