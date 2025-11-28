

#include "api.h"

float dot_product(const std::vector<float> &vec1,
                  const std::vector<float> &vec2)
{
    float result = 0.0f;
    for (size_t i = 0; i < vec1.size(); ++i)
    {
        result += vec1[i] * vec2[i];
    }
    return result;
}
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