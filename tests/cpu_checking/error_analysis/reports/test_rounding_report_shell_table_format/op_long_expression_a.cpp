#include "api.h"

float op_long_expression_a(int n) {
    float x = 0.1234567f;
    for (int i = 0; i < n; ++i) {
        x = x + (1.000001f * 0.33333334f + 2.000002f * 0.22222223f + 3.000003f * 0.11111112f + static_cast<float>(i) * 1e-7f);
    }
    return x;
}
