#include<iostream>
#include<cmath>
/*
 * computing sqrt(x) using Heron's algorithm
 */
void sqrtHeron()
{
    double x = 2;
    double r = x/2;

    while(1e-15 < std::abs(r*r - x))
    {
        r = (r + x/r) / 2;
        std::cout << r << std::endl;
    }
}

int main(){
  sqrtHeron();
}
