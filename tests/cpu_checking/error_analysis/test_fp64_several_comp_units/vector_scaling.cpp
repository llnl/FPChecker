
#include "api.h"

void vector_scaling(const std::vector<double> &vec,
                    double scalar,
                    std::vector<double> &result)
{
    for (size_t i = 0; i < vec.size(); ++i)
    {
        result[i] = vec[i] * scalar;
    }
}
void vector_scaling(const std::vector<long double> &vec,
                    double scalar,
                    std::vector<long double> &result)
{
    for (size_t i = 0; i < vec.size(); ++i)
    {
        long double scalar_d = static_cast<long double>(scalar);
        result[i] = vec[i] * scalar_d;
    }
}
