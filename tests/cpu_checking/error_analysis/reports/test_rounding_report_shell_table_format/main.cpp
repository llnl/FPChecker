#include <iostream>
#include "api.h"

int main() {
    const int n = 25000;

    volatile float sink = 0.0f;
    sink += op_long_expression_a(n);
    sink += op_long_expression_b(n);

    std::cout << "sink=" << sink << std::endl;
    return 0;
}
