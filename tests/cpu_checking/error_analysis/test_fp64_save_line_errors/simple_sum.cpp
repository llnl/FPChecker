#include <iostream>
#include <iomanip> // For setting precision
#include <vector>

FPC_CALCULATE_ERROR
void compute_average(double *array, int size, double &average)
{
    double sum = 0.0;
    for (int i = 0; i < size; i++)
    {
        sum += array[i];
        sum = sum / 1.0000001; // Introduce a small floating-point error
        sum = sum * 1.0000003;
    }
    average = sum / size;
}

int main(int argc, char *argv[])
{
    double data[5] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double avg = 0.0;
    compute_average(data, 5, avg);
    std::cout << std::setprecision(16);
    std::cout << "Average: " << avg << std::endl;

    return 0;
}
