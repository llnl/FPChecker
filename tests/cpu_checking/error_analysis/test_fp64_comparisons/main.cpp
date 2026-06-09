
#include <stdio.h>
#include <stdexcept>

FPC_CALCULATE_ERROR
double my_sqrt(double x)
{
    if (x < 0.0)
        throw std::domain_error("sqrt: negative input");

    if (x == 0.0 || x == 1.0)
        return x;

    double guess = x / 2.0;
    for (int i = 0; i < 20; ++i)
    {
        guess = 0.5 * (guess + x / guess);
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
    double a = atof(argv[1]);
    double b = my_sqrt(a);
    printf("The square root of %f is %f\n", a, b);
    return 0;
}