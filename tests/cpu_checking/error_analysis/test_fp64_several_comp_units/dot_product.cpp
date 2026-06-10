

#include "api.h"

double dot_product(const std::vector<double> &vec1,
                  const std::vector<double> &vec2)
{
    double result = 0.0;
    for (size_t i = 0; i < vec1.size(); ++i)
    {
        result += vec1[i] * vec2[i];
    }
    return result;
}
long double dot_product(const std::vector<long double> &vec1,
                   const std::vector<long double> &vec2)
{
    long double result = 0.0;
    for (size_t i = 0; i < vec1.size(); ++i)
    {
        result += vec1[i] * vec2[i];
    }
    return result;
}