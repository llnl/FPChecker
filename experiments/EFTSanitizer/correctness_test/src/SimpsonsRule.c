#include <stdio.h>
#include <stdlib.h>
#include <math.h>
extern void eftsan_print_error(double);
float f(float x) {
    return x * x;
}

float f2(float x) {
    return sqrtf(x) / (x * x - 1);
}

void floatSimpsonsRuleV1F1(float lrange, float urange, int n) {
    if (n % 2 != 0) {
        printf("n has to be an even number\n");
        return;
    }
    float step = (urange - lrange) / (float)n;
    
    float sum = f(lrange);
    for (int i = 1; i < n; i = i + 2) {
        // even-th term: 4 * f(x_{i})
        sum += 4 * f(lrange + (float)i * step);
        // odd-th term: 2 * f(x_{i+1})
        sum += 2 * f(lrange + (float)(i + 1) * step);
    }
    sum += f(urange);
    sum *= (step / 3.0);
    
    eftsan_print_error(sum);
    printf("V1: %.50e\n", sum);
}

void floatSimpsonsRuleV1F2(float lrange, float urange, int n) {
    if (n % 2 != 0) {
        printf("n has to be an even number\n");
        return;
    }
    float step = (urange - lrange) / (float)n;
    
    float sum = f2(lrange);
    for (int i = 1; i < n; i = i + 2) {
        // even-th term: 4 * f(x_{i})
        sum += 4 * f2(lrange + (float)i * step);
        // odd-th term: 2 * f(x_{i+1})
        sum += 2 * f2(lrange + (float)(i + 1) * step);
    }
    sum += f2(urange);
    sum *= (step / 3.0);
    
    eftsan_print_error(sum);
    printf("V1: %.50e\n", sum);
}
void floatSimpsonsRuleV2F1(float lrange, float urange, int n) {
    if (n % 2 != 0) {
        printf("n has to be an even number\n");
        return;
    }
    float step = (urange - lrange) / (float)n;
    
    float sum = f(lrange) * (step / 3.0);
    for (int i = 1; i < n; i = i + 2) {
        // even-th term: 4 * f(x_{i})
        sum += 4 * f(lrange + (float)i * step) * (step / 3.0);
        // odd-th term: 2 * f(x_{i+1})
        sum += 2 * f(lrange + (float)(i + 1) * step) * (step / 3.0);
    }
    sum += f(urange) * (step / 3.0);
    
    eftsan_print_error(sum);
    printf("V2: %.50e\n", sum);
}
void floatSimpsonsRuleV2F2(float lrange, float urange, int n) {
    if (n % 2 != 0) {
        printf("n has to be an even number\n");
        return;
    }
    float step = (urange - lrange) / (float)n;
    
    float sum = f2(lrange) * (step / 3.0);
    for (int i = 1; i < n; i = i + 2) {
        // even-th term: 4 * f(x_{i})
        sum += 4 * f2(lrange + (float)i * step) * (step / 3.0);
        // odd-th term: 2 * f(x_{i+1})
        sum += 2 * f2(lrange + (float)(i + 1) * step) * (step / 3.0);
    }
    sum += f2(urange) * (step / 3.0);
    
    eftsan_print_error(sum);
    printf("V2: %.50e\n", sum);
}

int main(int argc, char** argv) {
    floatSimpsonsRuleV1F1(13223113, 14223113, 20000000);
    
}
