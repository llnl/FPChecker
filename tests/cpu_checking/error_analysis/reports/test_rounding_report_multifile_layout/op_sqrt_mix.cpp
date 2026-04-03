#include <cmath>
#include "api.h"

float op_sqrt_mix(int n) {
    float x = 0.5f;
    for (int i = 1; i <= n; ++i) {
        x = std::sqrt(x + 0.12345f) * 1.00001f;
    }
    return x;
}
