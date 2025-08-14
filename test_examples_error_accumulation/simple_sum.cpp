#include <iostream>
#include <iomanip> // For setting precision
#include <vector>

// FPC_CALCULATE_ERROR
void compute_sum(int num_additions, float addend_f, float &sum_f)
{
    for (int i = 0; i < num_additions; ++i)
    {
        sum_f += addend_f;
    }
}

FPC_CALCULATE_ERROR
void compute_average(int num_additions, float avg_f, float &average)
{
    float sum_f = 0.0f;
    for (int i = 0; i < num_additions; ++i)
    {
        sum_f += avg_f;
    }
    average = sum_f / num_additions;
}

int main(int argc, char *argv[])
{
    // Define the number of additions
    int num_additions = atof(argv[1]);

    // The value we are adding has a representation error in float
    float addend_f = atof(argv[2]);
    double addend_d = atof(argv[3]);

    // Initialize sums
    float sum_f = 0.0f;
    double sum_d = 0.0;

    std::cout << "--- Process ---" << std::endl;
    std::cout << "Adding " << addend_f << " " << num_additions << " times..." << std::endl
              << std::endl;

    compute_sum(num_additions, addend_f, sum_f);

    // Perform the summations
    for (int i = 0; i < num_additions; ++i)
    {
        sum_d += addend_d;
    }

    // --- Output the results ---
    // Use std::setprecision to show enough decimal places to see the error
    std::cout << std::fixed << std::setprecision(10);

    std::cout << "--- Final Sums ---" << std::endl;
    std::cout << "Mathematical expected result: 1.0" << std::endl;
    std::cout << "Sum calculated in double:     " << sum_d << std::endl;
    // Cast the float result to double for an accurate printout
    std::cout << "Sum calculated in float:      " << static_cast<double>(sum_f) << std::endl;
    std::cout << std::endl;

    // 4. Calculate the total rounding error
    double total_error = sum_d - static_cast<double>(sum_f);

    std::cout << "--- Total Rounding Error ---" << std::endl;
    std::cout << "Total Error = (double sum) - (float sum)" << std::endl;
    std::cout << "Total Error: " << total_error << std::endl;

    // Show the error in scientific notation for clarity
    std::cout << std::scientific;
    std::cout << "Total Error (sci): " << total_error << std::endl;

    // ------ Average ------------

    // Perform the summations
    double sum_avg = 0.0f;
    for (int i = 0; i < num_additions; ++i)
    {
        sum_avg += addend_d;
    }
    double average = sum_avg / num_additions;
    std::cout << "Avg calculated in double:     " << average << std::endl;

    float average_f = 0.0f;
    compute_average(num_additions, addend_f, average_f);
    std::cout << "Avg calculated in float:      " << static_cast<double>(average_f) << std::endl;

    // Calculate the total rounding error
    double total_error_avg = average - static_cast<double>(average_f);

    std::cout << "--- Total Rounding Error ---" << std::endl;
    // Show the error in scientific notation for clarity
    std::cout << std::scientific;
    std::cout << "Total Error (sci): " << total_error_avg << std::endl;

    return 0;
}
