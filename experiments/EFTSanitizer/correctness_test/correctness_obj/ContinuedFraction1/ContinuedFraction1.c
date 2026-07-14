#include <stdio.h>
extern void eftsan_print_error(double);
// What happens when there is no infinity?
// This continueed fraction experiences divide by 0 for x = 1, 2, 3, and 4
// From William Kahan's commentary on "The End of Error ..."
void ContinuedFraction1(float x) {
    printf("Calculating Continued Fraction of a formula with input x = %lf\n", x);
    
    float temp = 2 / (x - 3);
    temp = x - 2 - temp;
    temp = 10 / temp;
    temp = x - 7 + temp;
    temp = 1 / temp;
    temp = x - 2 - temp;
    temp = 12 / temp;
    float fresult = 13 - temp;
    
    eftsan_print_error(fresult);
    printf("float result: %lf\n", fresult);
}

int main( int argc, const char* argv[] )
{
    ContinuedFraction1(1.0);
    ContinuedFraction1(2.0);
    ContinuedFraction1(3.0);
    ContinuedFraction1(4.0);
    return 0;
}
