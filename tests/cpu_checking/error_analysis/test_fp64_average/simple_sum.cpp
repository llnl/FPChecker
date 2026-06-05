#include <iostream>
#include <iomanip>
#include <cstdlib>

void compute_sum(int num_additions, double addend_d, double &sum_d)
{
    for (int i = 0; i < num_additions; ++i)
    {
        sum_d += addend_d;
    }
}

void compute_average(int num_additions, double avg_d, double &average)
{
    double sum_d = 0.0;
    for (int i = 0; i < num_additions; ++i)
    {
        sum_d += avg_d;
    }
    average = sum_d / num_additions;
}

int main(int argc, char *argv[])
{
    // int num_additions = atof(argv[1]);
    int num_additions = 100000;

    // The value we are adding has a representation error in double.
    // double addend_d = atof(argv[2]);
    double addend_d = 0.1;


    // Initialize sums.
    double sum_d = 0.0;
    long double sum_ld = 0.0L;

    // std::cout << "--- Process ---" << std::endl;
    // std::cout << "Adding " << addend_d << " " << num_additions << " times..." << std::endl
    //           << std::endl;

    compute_sum(num_additions, addend_d, sum_d);

    // Reference computation in long double.
    for (int i = 0; i < num_additions; ++i)
    {
        sum_ld += static_cast<long double>(addend_d);
    }

    // --- Output the results ---
    std::cout << std::fixed << std::setprecision(21);

    std::cout << "Sum calculated in long double: " << sum_ld << std::endl;
    std::cout << "Sum calculated in double:      " << static_cast<long double>(sum_d) << std::endl;
    std::cout << std::endl;

    // Calculate total rounding error.
    long double total_error = sum_ld - static_cast<long double>(sum_d);

    std::cout << "--- Total Rounding Error (SUM) ---" << std::endl;
    std::cout << "Total Error = (long double sum) - (double sum)" << std::endl;
    std::cout << "Total Error: " << total_error << std::endl;

    std::cout << std::scientific;
    std::cout << "Total Error (SUM): " << total_error << std::endl;

    // ------ Average ------------
    std::cout << std::endl;

    long double sum_avg_ld = 0.0L;
    for (int i = 0; i < num_additions; ++i)
    {
        sum_avg_ld += static_cast<long double>(addend_d);
    }

    long double average_ld = sum_avg_ld / static_cast<long double>(num_additions);
    std::cout << "Avg calculated in long double: " << average_ld << std::endl;

    double average_d = 0.0;
    compute_average(num_additions, addend_d, average_d);
    std::cout << "Avg calculated in double:      " << static_cast<long double>(average_d) << std::endl;

    // Calculate average rounding error.
    long double total_error_avg = average_ld - static_cast<long double>(average_d);

    std::cout << std::endl;
    std::cout << "--- Total Rounding Error (AVERAGE) ---" << std::endl;
    std::cout << std::scientific;
    std::cout << "Total Error (AVERAGE): " << total_error_avg << "\n"
              << std::endl;

    return 0;
}