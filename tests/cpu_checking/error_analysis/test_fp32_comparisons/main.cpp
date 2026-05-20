
#include <stdio.h>
#include <stdexcept>

FPC_CALCULATE_ERROR
float my_sqrt(float x)
{
    if (x < 0.0f)
        throw std::domain_error("sqrt: negative input");

    if (x == 0.0f || x == 1.0f)
        return x;

    float guess = x / 2.0f;
    for (int i = 0; i < 20; ++i)
    {
        guess = 0.5f * (guess + x / guess);
    }
    return guess;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: %s <number>\n", argv[0]);
        return 1;
    }
    float a = atof(argv[1]);
    float b = my_sqrt(a);
    printf("The square root of %f is %f\n", a, b);
    return 0;
}