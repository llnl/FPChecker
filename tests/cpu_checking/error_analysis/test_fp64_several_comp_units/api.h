#pragma once

#include <vector>
#include <cmath>

double dot_product(const std::vector<double> &vec1,
                  const std::vector<double> &vec2);
long double dot_product(const std::vector<long double> &vec1,
                   const std::vector<long double> &vec2);

void vector_addition(const std::vector<double> &vec1,
                     const std::vector<double> &vec2,
                     std::vector<double> &result);
void vector_addition(const std::vector<long double> &vec1,
                     const std::vector<long double> &vec2,
                     std::vector<long double> &result);

void vector_scaling(const std::vector<double> &vec,
                    double scalar,
                    std::vector<double> &result);
void vector_scaling(const std::vector<long double> &vec,
                    double scalar,
                    std::vector<long double> &result);
