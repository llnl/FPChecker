#include <iostream>
#include <vector>
#include <stdexcept>
#include <iomanip>

// Function to compute a final float value using a nested loop.
// The computation is a weighted sum of a 2D matrix.
FPC_CALCULATE_ERROR void computeNestedFloatValue_f(const std::vector<std::vector<float>> &matrix, const std::vector<float> &weights, float &result)
{
    float final_value = 0.0f;

    // Outer loop: Iterates through each row of the matrix.
    for (const auto &row : matrix)
    {
        float row_weighted_sum = 0.0f;

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

void computeNestedFloatValue_d(const std::vector<std::vector<float>> &matrix, const std::vector<float> &weights, double &result)
{
    double final_value = 0.0;

    for (const auto &row : matrix)
    {
        double row_weighted_sum = 0.0;

        for (size_t j = 0; j < row.size(); ++j)
        {
            row_weighted_sum += static_cast<double>(row[j]) * static_cast<double>(weights[j]);
        }

        final_value += row_weighted_sum;
    }

    result = final_value;
}

int main()
{
    // Example usage:
    // A 3x4 matrix representing data points.
    std::vector<std::vector<float>> data_matrix = {
        {1.5f, 2.0f, 3.5f, 4.0f},
        {5.0f, 6.5f, 7.0f, 8.5f},
        {9.0f, 10.5f, 11.0f, 12.5f}};

    // A 1D array of weights for each column.
    std::vector<float> weights_vector = {0.1f, 0.2f, 0.3f, 0.4f};

    // Compute the final value.
    float result_f = 0.0f;
    computeNestedFloatValue_f(data_matrix, weights_vector, result_f);
    double result_d = 0.0;
    computeNestedFloatValue_d(data_matrix, weights_vector, result_d);

    std::cout << "Data Matrix:" << std::endl;
    for (const auto &row : data_matrix)
    {
        for (float val : row)
        {
            std::cout << val << "\t";
        }
        std::cout << std::endl;
    }

    std::cout << "\nWeights Vector: ";
    for (float w : weights_vector)
    {
        std::cout << w << " ";
    }
    std::cout << std::endl;

    std::cout << std::fixed << std::setprecision(17);
    std::cout << std::scientific;

    std::cout << "Final computed float value: " << result_f << std::endl;
    std::cout << "Final computed double value: " << result_d << std::endl;
    double error = result_d - static_cast<double>(result_f);
    std::cout << "Total error (double): " << error << std::endl;

    return 0;
}