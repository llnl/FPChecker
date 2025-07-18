#include <iostream>
using namespace std;

void kernel(float *x, float *y)
{
    float result = 0.0f;
    float tmp = x[0] / y[1];
    y[2] = tmp;
    y[3] = tmp;
    y[4] = tmp;
    float tmp2 = y[4] * x[2];
    y[5] = tmp2;
}

int main(int argc, char **argv)
{
    float x[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    float y[] = {0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.5f};

    kernel(x, y);
    for (int i = 0; i < 6; ++i)
    {
        cout << "y[" << i << "] = " << y[i] << endl;
    }

    return 0;
}