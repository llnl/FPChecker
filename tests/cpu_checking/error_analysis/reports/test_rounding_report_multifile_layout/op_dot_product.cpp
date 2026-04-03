#include "api.h"

float op_dot_product(int n) {
    float sum = 0.0f;
    for (int i = 1; i <= n; ++i) {
        float x = 0.1001f * static_cast<float>(i);
        float y = 1.0f / (static_cast<float>(i) + 0.5f);
        sum = sum + x * y;
    }
    return sum;
}
