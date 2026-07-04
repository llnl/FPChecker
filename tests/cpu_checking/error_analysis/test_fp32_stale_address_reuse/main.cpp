#include <cmath>
#include <iomanip>
#include <iostream>

extern "C" void overwrite_float(float *dst, float value);

FPC_CALCULATE_ERROR
float compute_with_external_overwrite()
{
    float a = 0.1f;
    float b = 0.2f;
    float slot = a * b;

    overwrite_float(&slot, 7.25f);

    float loaded = slot;
    float scale = 1.1f;
    float bias = 0.33333334f;
    float result = (loaded * scale) + bias; // FINAL_RESULT
    return result;
}

double compute_reference()
{
    float loaded_f = 7.25f;
    float scale_f = 1.1f;
    float bias_f = 0.33333334f;
    return (static_cast<double>(loaded_f) * static_cast<double>(scale_f)) +
           static_cast<double>(bias_f);
}

int main()
{
    float result = compute_with_external_overwrite();
    double reference = compute_reference();
    double difference = reference - static_cast<double>(result);

    std::cout << std::scientific << std::setprecision(17);
    std::cout << "Result: " << result << "\n";
    std::cout << "Reference: " << reference << "\n";
    std::cout << "Difference: " << difference << "\n";
    return 0;
}
