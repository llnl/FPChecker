#include <iostream>
#include "api.h"

int main() {
    const int n = 20000;

    volatile float sink = 0.0f;
    sink += op_addition(n);
    sink += op_subtraction(n);
    sink += op_multiplication(n);
    sink += op_division(n);
    sink += op_fma_chain(n);
    sink += op_reciprocal_sum(n);
    sink += op_horner(n);
    sink += op_sqrt_mix(n);
    sink += op_lerp(n);
    sink += op_dot_product(n);

    std::cout << "sink=" << sink << std::endl;
    return 0;
}
