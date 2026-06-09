#include <iostream>

#ifdef ADD_FUNCTION
FPC_CALCULATE_ERROR
#endif
void function_add(double *array, int size, double &result)
{
    double x = array[0] + array[1];
    double y = x + array[2];
    double z = y + array[3];

    result = z;
}

#ifdef SUB_FUNCTION
FPC_CALCULATE_ERROR
#endif
void function_sub(double *array, int size, double &result)
{
    double x = array[0] - array[1];
    double y = x - array[2];
    double z = y - array[3];

    result = z;
}

#ifdef MUL_FUNCTION
FPC_CALCULATE_ERROR
#endif
void function_mul(double *array, int size, double &result)
{
    double x = array[0] * array[1];
    double y = x * array[2];
    double z = y * array[3];

    result = z;
}

#ifdef DIV_FUNCTION
FPC_CALCULATE_ERROR
#endif
void function_div(double *array, int size, double &result)
{
    double x = array[0] / array[1];
    double y = x / array[2];
    double z = y / array[3];

    result = z;
}

int main(int argc, char *argv[])
{
    double array[4] = {1.3, 2.3, 3.3, 4.3};
    double result = 0.0;

    std::cout.precision(17);
    std::cout << std::scientific;
    function_add(array, 4, result);
    std::cout << "Addition result: " << result << std::endl;
    result = 0.0;
    function_sub(array, 4, result);
    std::cout << "Subtraction result: " << result << std::endl;
    result = 0.0;
    function_mul(array, 4, result);
    std::cout << "Multiplication result: " << result << std::endl;
    result = 0.0;
    function_div(array, 4, result);
    std::cout << "Division result: " << result << std::endl;

    return 0;
}