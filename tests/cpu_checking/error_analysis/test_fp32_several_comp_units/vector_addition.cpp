
#include "api.h"

void vector_addition(const std::vector<float> &vec1,
                     const std::vector<float> &vec2,
                     std::vector<float> &result)
{
    for (size_t i = 0; i < vec1.size(); ++i)
    {
        result[i] = vec1[i] + vec2[i];
    }
}
void vector_addition(const std::vector<double> &vec1,
                     const std::vector<double> &vec2,
                     std::vector<double> &result)
{
    for (size_t i = 0; i < vec1.size(); ++i)
    {
        result[i] = vec1[i] + vec2[i];
    }
}