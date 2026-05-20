#include "api.h"

float op_division(int n) {
    float x = 1.2345678f;
    for (int i = 1; i <= n; ++i) {
        x = x + 0.0001f;
        x = x / 1.0003f;
    }
    return x;
}
