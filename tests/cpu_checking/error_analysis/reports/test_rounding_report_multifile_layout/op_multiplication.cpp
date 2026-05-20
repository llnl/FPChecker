#include "api.h"

float op_multiplication(int n) {
    float x = 1.00001f;
    for (int i = 0; i < n; ++i) {
        x = x * 1.0001f;
    }
    return x;
}
