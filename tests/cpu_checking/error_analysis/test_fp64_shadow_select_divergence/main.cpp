#include <iomanip>
#include <iostream>

FPC_CALCULATE_ERROR
double compute_select()
{
    volatile double source_large = 9007199254740992.0;
    volatile double source_one = 1.0;

    double large = source_large;
    double one = source_one;
    double x = (large + one) - large;
    double selected = (x > 0.5) ? 10.0 : 20.0; // SHADOW_SELECT
    double result = selected + 0.25;            // FINAL_RESULT
    return result;
}

long double compute_reference()
{
    double large_d = 9007199254740992.0;
    double one_d = 1.0;
    long double large = static_cast<long double>(large_d);
    long double one = static_cast<long double>(one_d);
    long double x = (large + one) - large;
    long double selected = (x > 0.5L) ? 10.0L : 20.0L;
    return selected + static_cast<long double>(0.25);
}

int main()
{
    double result = compute_select();
    long double reference = compute_reference();
    long double difference = reference - static_cast<long double>(result);

    std::cout << std::scientific << std::setprecision(21);
    std::cout << "Result: " << result << "\n";
    std::cout << "Reference: " << reference << "\n";
    std::cout << "Difference: " << difference << "\n";
    return 0;
}
