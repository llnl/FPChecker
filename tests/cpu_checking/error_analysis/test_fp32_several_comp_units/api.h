#pragma once

#include <vector>
#include <cmath>

float dot_product(const std::vector<float> &vec1,
                  const std::vector<float> &vec2);
double dot_product(const std::vector<double> &vec1,
                   const std::vector<double> &vec2);

void vector_addition(const std::vector<float> &vec1,
                     const std::vector<float> &vec2,
                     std::vector<float> &result);
void vector_addition(const std::vector<double> &vec1,
                     const std::vector<double> &vec2,
                     std::vector<double> &result);

void vector_scaling(const std::vector<float> &vec,
                    float scalar,
                    std::vector<float> &result);
void vector_scaling(const std::vector<double> &vec,
                    float scalar,
                    std::vector<double> &result);
