#include "api.h"

#include <algorithm>
#include <cstddef>

std::vector<double> transform_values(std::vector<double> values)
{
    std::vector<double> copy(values.begin(), values.end());
    copy.push_back((values[0] * 1.25) + 0.03125);

    std::vector<double> out(copy.size());
    std::copy(copy.begin(), copy.end(), out.begin());

    for (std::size_t i = 0; i < out.size(); ++i)
    {
        double scale = 1.0000000000000002 +
                       static_cast<double>(i) * 0.00000000000000022204;
        out[i] = (out[i] * scale) + values[i % values.size()];
    }
    return out;
}

std::vector<long double> transform_values(std::vector<long double> values)
{
    std::vector<long double> copy(values.begin(), values.end());
    copy.push_back((values[0] * static_cast<long double>(1.25)) +
                   static_cast<long double>(0.03125));

    std::vector<long double> out(copy.size());
    std::copy(copy.begin(), copy.end(), out.begin());

    for (std::size_t i = 0; i < out.size(); ++i)
    {
        long double scale = static_cast<long double>(1.0000000000000002) +
                            static_cast<long double>(static_cast<double>(i)) *
                                static_cast<long double>(0.00000000000000022204);
        out[i] = (out[i] * scale) + values[i % values.size()];
    }
    return out;
}
