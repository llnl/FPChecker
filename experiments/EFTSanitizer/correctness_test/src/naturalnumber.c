#include <stdio.h>
#include <math.h>
extern void eftsan_print_error(double);
// Float: 2.7182819843292236328125
// Real : 2.7182818284590452353602874713527...
// Posit: 2.7182818353176116943359375
int main(int argc, char** argv) {
    float e = 2.0f;
    float prevE = 0.0f;
    
    float step = 2.0f;
    float prevStep = 0.0f;
    
    float denom = 2.0f;
    while (e != prevE && step != prevStep) {
        float term = 1.0f / denom;
	prevE = e;
        e += term;
        prevStep = step;
        step += 1.0f;
        denom *= step;
    }
    eftsan_print_error(e);
    printf("%.50e\n", e);
}
