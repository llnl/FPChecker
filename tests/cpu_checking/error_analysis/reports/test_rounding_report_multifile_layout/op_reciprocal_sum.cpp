#include "api.h"

float op_reciprocal_sum(int n) {
    float s = 0.0f;
    for (int i = 1; i <= n; ++i) {
        s = s + 1.0f / (static_cast<float>(i) + 0.37f);
    }
    return s;
}
