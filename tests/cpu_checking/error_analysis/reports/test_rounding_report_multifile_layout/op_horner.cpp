#include "api.h"

float op_horner(int n) {
    float x = 0.001f;
    float p = 0.0f;
    for (int i = 0; i < n; ++i) {
        p = (((0.1251f * x + 0.3313f) * x + 0.9917f) * x + 1.7131f) * x + 0.1177f;
        x = x + 1e-5f;
    }
    return p;
}
