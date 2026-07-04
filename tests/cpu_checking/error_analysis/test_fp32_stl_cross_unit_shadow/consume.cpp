#include "api.h"

#include <cstddef>

float consume_values(const std::vector<float> &values)
{
    float sum = 0.0f;
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        sum = (sum + values[i]) * 0.99975586f;
    }
    return sum;
}

double consume_values(const std::vector<double> &values)
{
    double sum = 0.0;
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        sum = (sum + values[i]) * static_cast<double>(0.99975586f);
    }
    return sum;
}
