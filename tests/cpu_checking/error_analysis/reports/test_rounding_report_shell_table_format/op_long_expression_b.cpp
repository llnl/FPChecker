#include "api.h"

float op_long_expression_b(int n) {
    float y = 1.0000001f;
    for (int i = 1; i <= n; ++i) {
        y = y + ((4.000004f / (0.1234567f + static_cast<float>(i) * 1e-6f)) - (2.000002f / (0.7654321f + static_cast<float>(i) * 2e-6f)));
    }
    return y;
}
