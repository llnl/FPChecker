#include "api.h"

float op_addition(int n) {
    float x = 0.0f;
    for (int i = 0; i < n; ++i) {
        x = x + 0.1f;
    }
    return x;
}
