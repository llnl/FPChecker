#include "api.h"

float op_fma_chain(int n) {
    float a = 1.00031f;
    float b = 0.99991f;
    float c = 0.1234567f;
    float x = 0.0f;
    for (int i = 0; i < n; ++i) {
        x = x + (a * b + c);
        c = c * 0.99999f;
    }
    return x;
}
