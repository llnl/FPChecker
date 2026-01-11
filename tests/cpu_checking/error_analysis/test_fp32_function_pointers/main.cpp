
#include <iostream>
#include <cstddef>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <dlfcn.h>

//__attribute__((noinline))

void parse_input(int *argc, char **argv[], std::vector<float> &values_f, std::vector<double> &values_d, char *mode)
{
    if (*argc < 2)
    {
        std::cerr << "Usage: " << (*argv)[0] << " <comma_separated_float_values>" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (mode != nullptr)
    {
        strcpy(mode, (*argv)[2]);
    }
    else
    {
        exit(EXIT_FAILURE);
    }

    std::string input = (*argv)[1];
    std::stringstream ss(input);
    std::string item;

    while (std::getline(ss, item, ','))
    {
        try
        {
            values_f.push_back(std::stof(item));
        }
        catch (const std::invalid_argument &)
        {
            std::cerr << "Invalid float value: " << item << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // Print the parsed float values
    std::cout << "Parsed input float values:" << std::endl;
    for (const auto &val : values_f)
    {
        std::cout << val << std::endl;
        values_d.push_back(static_cast<double>(val));
    }
}

__attribute__((noinline)) void calc_dot_product_f(const float *a, const float *b, size_t n, float &result)
{
    float res = 0.0f;
    for (size_t i = 0; i < n; ++i)
    {
        res += a[i] * b[i];
    }

    result = res;
}

__attribute__((noinline)) void calc_dot_product_scaled_f(const float *a, const float *b, size_t n, float &result)
{
    float res = 0.0f;
    for (size_t i = 0; i < n; ++i)
    {
        res += a[i] * b[i];
        res *= a[0]; // Introduce slight scaling to increase error
    }

    result = res;
}

__attribute__((noinline)) void calc_dot_product_scaled_d(const double *a, const double *b, size_t n, double &result)
{
    double res = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        res += a[i] * b[i];
        res *= a[0]; // Introduce slight scaling to increase error
    }

    result = res;
}

// Provide C-linkage wrappers with simple pointer result parameter so we can
// reliably lookup them by name with dlsym at runtime (C names are unmangled).
extern "C" void calc_dot_product_f_c(const float *a, const float *b, size_t n, float *result)
{
    float &r = *result;
    calc_dot_product_f(a, b, n, r);
}

extern "C" void calc_dot_product_scaled_f_c(const float *a, const float *b, size_t n, float *result)
{
    float &r = *result;
    calc_dot_product_scaled_f(a, b, n, r);
}

int main(int argc, char *argv[])
{
    std::vector<float> values_f;
    std::vector<double> values_d;
    char mode[100];
    mode[0] = '\0';
    parse_input(&argc, &argv, values_f, values_d, mode);

    using dot_fn_t = void (*)(const float *, const float *, size_t, float *);
    dot_fn_t dot_fn = nullptr;

    // Resolve the function pointer at runtime using dlsym so the compiler
    // cannot statically determine which function will be called.
    const char *symName = (std::strcmp(mode, "scaled") == 0) ? "calc_dot_product_scaled_f_c" : "calc_dot_product_f_c";
    void *handle = dlopen(nullptr, RTLD_LAZY);
    if (!handle)
    {
        std::cerr << "dlopen failed: " << dlerror() << std::endl;
        return 1;
    }
    dlerror(); // clear any existing error
    dot_fn = reinterpret_cast<dot_fn_t>(dlsym(handle, symName));
    char *dlsym_err = dlerror();
    if (dlsym_err)
    {
        std::cerr << "dlsym failed: " << dlsym_err << std::endl;
        return 1;
    }

    size_t n = values_f.size();
    if (n == 0)
    {
        std::cerr << "No input values provided" << std::endl;
        return 0;
    }

    float result_f = 0.0f;
    // use the same vector for both operands (dot product with itself)
    dot_fn(values_f.data(), values_f.data(), n, &result_f);

    // scientific notation with 17 decimal places
    std::cout << std::fixed << std::setprecision(17);
    std::cout << std::scientific;
    std::cout << "Dot product (" << mode << "): " << result_f << std::endl;

    float val_sum_f = 0.0f;
    for (size_t i = 0; i < values_f.size(); ++i)
    {
        val_sum_f += values_f[i] + result_f;
    }

    std::cout << "Final sum (float): " << val_sum_f << std::endl;

    // Compute in double for reference
    double result_d = 0.0;
    calc_dot_product_scaled_d(values_d.data(), values_d.data(), n, result_d);
    std::cout << "Dot product in double (reference): " << result_d << std::endl;

    double val_sum_d = 0.0;
    for (size_t i = 0; i < values_d.size(); ++i)
    {
        val_sum_d += values_d[i] + result_d;
    }

    std::cout << "Final sum (double): " << val_sum_d << std::endl;

    // Difference
    double diff = val_sum_d - static_cast<double>(val_sum_f);
    std::cout << "Difference: " << diff << std::endl;

    // Close the handle
    dlclose(handle);

    return 0;
}