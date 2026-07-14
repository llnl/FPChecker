#include <array>
#include <cmath>
#include <iostream>

//-----------------------------------------------------------------------------
// POLYNOMIAL EVALUATION

// return the coefiscients of the 20th legendre polynomial
std::array<double, 21> legendre20 = []()
{
    std::array<double, 21> result;

    result[20] = double(34461632205) / double(262144);
    result[19] = 0.;
    result[18] = double(-83945001525) / double(131072);
    result[17] = 0.;
    result[16] = double(347123925225) / double(262144);
    result[15] = 0.;
    result[14] = double(-49589132175) / double(32768);
    result[13] = 0.;
    result[12] = double(136745788725) / double(131072);
    result[11] = 0.;
    result[10] = double(-29113619535) / double(65536);
    result[9] = 0.;
    result[8] = double(15058768725) / double(131072);
    result[7] = 0.;
    result[6] = double(-557732175) / double(32768);
    result[5] = 0.;
    result[4] = double(334639305) / double(262144);
    result[3] = 0.;
    result[2] = double(-4849845) / double(131072);
    result[1] = 0.;
    result[0] = double(46189) / double(262144);

    return result;
}();

//-----

// compute a a value using Horner's algorithm
double legendreHorner(double x)
{
    double result = 0.0;

    for (int i = 20; i >= 0; i--)
    {
        double coef = legendre20[i];
        result = result*x + coef;

        // TODO it might be interesting to implement this operation with an FMA to see the impact on precision
        // result = fma(result,x,coef);
    }

    return result;
}

// compute a value using the naive polynomial evaluation algorithm
double legendreNaive(double x)
{
    double result = 0.0;

    for (int i = 20; i >= 0; i--)
    {
        double coef = legendre20[i];
        result += coef * std::pow(x,double(i));
    }

    return result;
}

// computes a value using the recurence formula
double legendreRec(int q, double x)
{
    if (q == 0)
    {
        return 1.0;
    }
    else if (q == 1)
    {
        return x;
    }
    else
    {
        double p0 = 1.0;
        double p1 = x;
        double pn = x;

        for (int n = 2; n <= q; n++)
        {
            //pn = (1/double(n)) * (x*(2*n-1)*p1 - (n-1)*p0);
            pn = (x * double(2*n-1) * p1 - double(n-1) * p0) / double(n);
            p0 = p1;
            p1 = pn;
        }

        return pn;
    }
}

//-----------------------------------------------------------------------------
// TESTS

/*
 * test on different evaluation methods for the legendre polynomials applied to a given x
 */
void legendre20Testx(double x)
{
    std::cout << "x = " << x << '\n'
              << "P_naive(x)\t=\t" << legendreNaive(x) << '\n'
              << "P_horner(x)\t=\t" << legendreHorner(x) << '\n'
              << "P_rec(x)\t=\t" << legendreRec(20, x) << '\n' << std::endl;
}

/*
 * test on different evaluation methods for the legendre polynomials
 * the Horner's method loses accuracy as we get closer to 1
 * meanwhile the recurcive method stays very accurate
 *
 * inspired by "some experiments with Evaluation of Legendre Polynomials"
 */
int main()
{
    std::cout << "It has been observed that Horner's algorithm lose most of its significativ digits while computing legendre's polynomial around 1-." << '\n'
              << "Can we reproduce those observations ?" << std::endl;

    legendre20Testx(0.5);
    legendre20Testx(0.6);
    legendre20Testx(0.8);
    legendre20Testx(0.9);
    legendre20Testx(0.99);
    legendre20Testx(0.999);
}
