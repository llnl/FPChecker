

#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>
#include <iomanip>

// Function to compute the sum of squares of elements in a vector
FPC_CALCULATE_ERROR
double compute_sum_of_squares(const std::vector<double> &data)
{
    double result = std::accumulate(
        data.begin(),
        data.end(),
        0.0, // Initial value for the sum
        [](double total, double current_element)
        {
            return total + (current_element * current_element);
        });
    return result;
}

int main()
{
    // 1. Define the data container (std::vector<double>)
    std::vector<double> numbers = {1.5, 2.0, 3.5, 4.0, 0.5};

    std::cout << "Data points: ";
    for (double n : numbers)
    {
        std::cout << n << " ";
    }
    std::cout << "\n\n";

    // 2. Call the function to perform the floating-point computation
    double sum_sq = compute_sum_of_squares(numbers);

    // 3. Print the result
    // Use std::fixed and std::setprecision for clean floating-point output
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "The computed sum of squares is: " << sum_sq << std::endl;

    return 0;
}