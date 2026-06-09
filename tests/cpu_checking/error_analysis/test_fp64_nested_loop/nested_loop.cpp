#include <iostream>
#include <vector>
#include <stdexcept>
#include <iomanip>

// Function to compute a final double value using a nested loop.
// The computation is a weighted sum of a 2D matrix.
__attribute__((noinline))
void computeNestedFloatValue_f(const std::vector<std::vector<double>> &matrix, const std::vector<double> &weights, double &result)
{
    double final_value = 0.0;

    // Outer loop: Iterates through each row of the matrix.
    for (const auto &row : matrix)
    {
        double row_weighted_sum = 0.0;

        // Inner loop: Iterates through each element in the current row
        // and multiplies it by the corresponding weight.
        for (size_t j = 0; j < row.size(); ++j)
        {
            row_weighted_sum += row[j] * weights[j];
        }

        // Add the weighted sum of the current row to the final value.
        final_value += row_weighted_sum;
    }

    result = final_value;
}

void computeNestedFloatValue_d(const std::vector<std::vector<double>> &matrix, const std::vector<double> &weights, long double &result)
{
    long double final_value = 0.0;

    for (const auto &row : matrix)
    {
        long double row_weighted_sum = 0.0;

        for (size_t j = 0; j < row.size(); ++j)
        {
            row_weighted_sum += static_cast<long double>(row[j]) * static_cast<long double>(weights[j]);
        }

        final_value += row_weighted_sum;
    }

    result = final_value;
}

int main()
{
    // Example usage:
    // A 3x4 matrix representing data points.
    std::vector<std::vector<double>> data_matrix = {
        {1.5, 2.0, 3.5, 4.0},
        {5.0, 6.5, 7.0, 8.5},
        {9.0, 10.5, 11.0, 12.5}};

    // A 1D array of weights for each column.
    std::vector<double> weights_vector = {0.1, 0.2, 0.3, 0.4};

    // Compute the final value.
    double result_f = 0.0;
    computeNestedFloatValue_f(data_matrix, weights_vector, result_f);
    long double result_d = 0.0;
    computeNestedFloatValue_d(data_matrix, weights_vector, result_d);

    std::cout << "Data Matrix:" << std::endl;
    for (const auto &row : data_matrix)
    {
        for (double val : row)
        {
            std::cout << val << "\t";
        }
        std::cout << std::endl;
    }

    std::cout << "\nWeights Vector: ";
    for (double w : weights_vector)
    {
        std::cout << w << " ";
    }
    std::cout << std::endl;

    std::cout << std::fixed << std::setprecision(17);
    std::cout << std::scientific;

    std::cout << "Final computed double value: " << result_f << std::endl;
    std::cout << "Final computed long double value: " << result_d << std::endl;
    long double error = result_d - static_cast<long double>(result_f);
    std::cout << "Total error (long double): " << error << std::endl;

    // Print relative error
    long double relative_error = (error != 0.0) ? std::abs(error / result_d) : 0.0;
    std::cout << "Relative error: " << relative_error << std::endl;

    return 0;
}