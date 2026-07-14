#include <stdio.h>
#include <math.h>
extern void eftsan_print_error(double);
// sqrt(b^2 - 4ac) is correct. Even -b +/- sqrt(b^2 - 4ac) might look correct.
// But then the roots turn out to be completely wrong.
int main( int argc, const char* argv[] )
{
    printf("rootfinding test case 2\n");
    float fa = 1;
    float fb = 54.32;
    float fc = 0.1;
    
    float fbb = fb * fb;
    float f4ac = 4.0 * fa * fc;
    float fresult = fbb - f4ac;
    fresult = sqrtf(fresult);
    float fnumer1 = -1.0 * fb + fresult;
    float fnumer2 = -1.0 * fb - fresult;
    float froot1 = fnumer1 / (2.0 * fa);
    float froot2 = fnumer2 / (2.0 * fa);

    eftsan_print_error(froot1);
    eftsan_print_error(froot2);
    printf("float root1: %.50e\n", froot1);
    printf("float root2: %.50e\n", froot2);
    return 0;
}
