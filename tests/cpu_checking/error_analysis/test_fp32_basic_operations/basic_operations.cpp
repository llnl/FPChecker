#include <iostream>

#ifdef ADD_FUNCTION
FPC_CALCULATE_ERROR
#endif
void function_add(float *array, int size, float &result)
{
    float x = array[0] + array[1];
    float y = x + array[2];
    float z = y + array[3];

    result = z;
}

#ifdef SUB_FUNCTION
FPC_CALCULATE_ERROR
#endif
void function_sub(float *array, int size, float &result)
{
    float x = array[0] - array[1];
    float y = x - array[2];
    float z = y - array[3];

    result = z;
}

#ifdef MUL_FUNCTION
FPC_CALCULATE_ERROR
#endif
void function_mul(float *array, int size, float &result)
{
    float x = array[0] * array[1];
    float y = x * array[2];
    float z = y * array[3];

    result = z;
}

#ifdef DIV_FUNCTION
FPC_CALCULATE_ERROR
#endif
void function_div(float *array, int size, float &result)
{
    float x = array[0] / array[1];
    float y = x / array[2];
    float z = y / array[3];

    result = z;
}

int main(int argc, char *argv[])
{
    float array[4] = {1.3, 2.3, 3.3, 4.3};
    float result = 0.0f;

    std::cout.precision(17);
    std::cout << std::scientific;
    function_add(array, 4, result);
    std::cout << "Addition result: " << result << std::endl;
    result = 0.0f;
    function_sub(array, 4, result);
    std::cout << "Subtraction result: " << result << std::endl;
    result = 0.0f;
    function_mul(array, 4, result);
    std::cout << "Multiplication result: " << result << std::endl;
    result = 0.0f;
    function_div(array, 4, result);
    std::cout << "Division result: " << result << std::endl;

    return 0;
}