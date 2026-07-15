#include <iomanip>
#include <iostream>

FPC_CALCULATE_ERROR
float compute_branch()
{
    volatile float source_large = 16777216.0f;
    volatile float source_one = 1.0f;

    float large = source_large;
    float one = source_one;
    float x = (large + one) - large;
    float selected = 0.0f;

    if (x > 0.5f) { // SHADOW_BRANCH
        selected = 10.0f;
    } else {
        selected = 20.0f;
    }

    float result = selected + 0.25f; // FINAL_RESULT
    return result;
}

double compute_reference()
{
    float large_f = 16777216.0f;
    float one_f = 1.0f;
    double large = static_cast<double>(large_f);
    double one = static_cast<double>(one_f);
    double x = (large + one) - large;
    double selected = (x > 0.5) ? 10.0 : 20.0;
    return selected + static_cast<double>(0.25f);
}

int main()
{
    float result = compute_branch();
    double reference = compute_reference();
    double difference = reference - static_cast<double>(result);

    std::cout << std::scientific << std::setprecision(17);
    std::cout << "Result: " << result << "\n";
    std::cout << "Reference: " << reference << "\n";
    std::cout << "Difference: " << difference << "\n";
    return 0;
}
