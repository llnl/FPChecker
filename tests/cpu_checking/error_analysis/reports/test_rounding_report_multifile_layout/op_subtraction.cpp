#include "api.h"

float op_subtraction(int n) {
    float x = 1.0f;
    for (int i = 0; i < n; ++i) {
        float y = 1000.1234f + static_cast<float>(i) * 1e-4f;
        x = x + y;
        x = x - y;
    }
    return x;
}
