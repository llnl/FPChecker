#include "api.h"

#include <cstddef>

double consume_values(const std::vector<double> &values)
{
    double sum = 0.0;
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        sum = (sum + values[i]) * 0.99999999999999989;
    }
    return sum;
}

long double consume_values(const std::vector<long double> &values)
{
    long double sum = 0.0L;
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        sum = (sum + values[i]) * static_cast<long double>(0.99999999999999989);
    }
    return sum;
}
