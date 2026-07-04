#include "api.h"

#include <algorithm>
#include <cstddef>

std::vector<float> transform_values(std::vector<float> values)
{
    std::vector<float> copy(values.begin(), values.end());
    copy.push_back((values[0] * 1.25f) + 0.03125f);

    std::vector<float> out(copy.size());
    std::copy(copy.begin(), copy.end(), out.begin());

    for (std::size_t i = 0; i < out.size(); ++i)
    {
        float scale = 1.0009766f + static_cast<float>(i) * 0.00024414062f;
        out[i] = (out[i] * scale) + values[i % values.size()];
    }
    return out;
}

std::vector<double> transform_values(std::vector<double> values)
{
    std::vector<double> copy(values.begin(), values.end());
    copy.push_back((values[0] * static_cast<double>(1.25f)) +
                   static_cast<double>(0.03125f));

    std::vector<double> out(copy.size());
    std::copy(copy.begin(), copy.end(), out.begin());

    for (std::size_t i = 0; i < out.size(); ++i)
    {
        double scale = static_cast<double>(1.0009766f) +
                       static_cast<double>(static_cast<float>(i)) *
                           static_cast<double>(0.00024414062f);
        out[i] = (out[i] * scale) + values[i % values.size()];
    }
    return out;
}
