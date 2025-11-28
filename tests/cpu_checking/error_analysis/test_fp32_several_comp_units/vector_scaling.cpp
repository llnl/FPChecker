
#include "api.h"

void vector_scaling(const std::vector<float> &vec,
                    float scalar,
                    std::vector<float> &result)
{
    for (size_t i = 0; i < vec.size(); ++i)
    {
        result[i] = vec[i] * scalar;
    }
}
void vector_scaling(const std::vector<double> &vec,
                    float scalar,
                    std::vector<double> &result)
{
    for (size_t i = 0; i < vec.size(); ++i)
    {
        double scalar_d = static_cast<double>(scalar);
        result[i] = vec[i] * scalar_d;
    }
}
