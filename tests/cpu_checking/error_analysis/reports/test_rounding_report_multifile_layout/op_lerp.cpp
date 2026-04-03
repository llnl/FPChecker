#include "api.h"

float op_lerp(int n) {
    float a = 1.1111111f;
    float b = 9.9999990f;
    float sum = 0.0f;
    for (int i = 0; i < n; ++i) {
        float t = static_cast<float>(i % 1000) / 999.0f;
        sum = sum + (a + t * (b - a));
    }
    return sum;
}
