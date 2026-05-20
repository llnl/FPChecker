#include <iostream>
#include <iomanip> // For setting precision
#include <vector>

FPC_CALCULATE_ERROR
void compute_average(float *array, int size, float &average)
{
    float sum = 0.0f;
    for (int i = 0; i < size; i++)
    {
        sum += array[i];
        sum = sum / 1.0000001f; // Introduce a small floating-point error
        sum = sum * 1.0000003f;
    }
    average = sum / size;
}

int main(int argc, char *argv[])
{
    float data[5] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    float avg = 0.0f;
    compute_average(data, 5, avg);
    std::cout << std::setprecision(16);
    std::cout << "Average: " << avg << std::endl;

    return 0;
}
