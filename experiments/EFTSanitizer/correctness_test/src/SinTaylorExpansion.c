#include <stdio.h>
#include <math.h>
extern void eftsan_print_error(double);
// I expected taylor expansions to have trouble, but it looks like they do not!
// Posit is almost as good as float here.
int main( int argc, const char* argv[] )
{
    float fx = 0.875f;
    
    float fcoeff[7] = {
        1.0,
        -1.6666667163372039794921875e-01,
        8.333333767950534820556640625e-03,
        -1.98412701138295233249664306640625e-04,
        2.755731884462875314056873321533203125e-06,
        -2.505210972003624192439019680023193359375e-08,
        1.605904298429550181026570498943328857421875e-10
    };
    
    // Compute Taylor's expansion of sin:
    float fx2 = fx * fx;
    float fxaccum = 0.875f;
    float fresult = 0;
    for (int i = 0; i < 7; i++){
        fresult += fcoeff[i] * fxaccum;
        fxaccum *= fx2;
    }

    eftsan_print_error(fresult);
    printf("float    result: %.50e\n", fresult);
    return 0;
}
