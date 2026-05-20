
#include <iostream>

typedef float Real_t;

static FPC_CALCULATE_ERROR void CalcElemShapeFunctionDerivatives(Real_t const x[],
                                                                 Real_t const y[],
                                                                 Real_t const z[],
                                                                 Real_t b[][8],
                                                                 Real_t *const volume)
{
    const Real_t x0 = x[0];
    const Real_t x1 = x[1];
    const Real_t x2 = x[2];
    const Real_t x3 = x[3];
    const Real_t x4 = x[4];
    const Real_t x5 = x[5];
    const Real_t x6 = x[6];
    const Real_t x7 = x[7];

    const Real_t y0 = y[0];
    const Real_t y1 = y[1];
    const Real_t y2 = y[2];
    const Real_t y3 = y[3];
    const Real_t y4 = y[4];
    const Real_t y5 = y[5];
    const Real_t y6 = y[6];
    const Real_t y7 = y[7];

    const Real_t z0 = z[0];
    const Real_t z1 = z[1];
    const Real_t z2 = z[2];
    const Real_t z3 = z[3];
    const Real_t z4 = z[4];
    const Real_t z5 = z[5];
    const Real_t z6 = z[6];
    const Real_t z7 = z[7];

    Real_t fjxxi, fjxet, fjxze;
    Real_t fjyxi, fjyet, fjyze;
    Real_t fjzxi, fjzet, fjzze;
    Real_t cjxxi, cjxet, cjxze;
    Real_t cjyxi, cjyet, cjyze;
    Real_t cjzxi, cjzet, cjzze;

    fjxxi = Real_t(.125) * ((x6 - x0) + (x5 - x3) - (x7 - x1) - (x4 - x2));
    fjxet = Real_t(.125) * ((x6 - x0) - (x5 - x3) + (x7 - x1) - (x4 - x2));
    fjxze = Real_t(.125) * ((x6 - x0) + (x5 - x3) + (x7 - x1) + (x4 - x2));

    fjyxi = Real_t(.125) * ((y6 - y0) + (y5 - y3) - (y7 - y1) - (y4 - y2));
    fjyet = Real_t(.125) * ((y6 - y0) - (y5 - y3) + (y7 - y1) - (y4 - y2));
    fjyze = Real_t(.125) * ((y6 - y0) + (y5 - y3) + (y7 - y1) + (y4 - y2));

    fjzxi = Real_t(.125) * ((z6 - z0) + (z5 - z3) - (z7 - z1) - (z4 - z2));
    fjzet = Real_t(.125) * ((z6 - z0) - (z5 - z3) + (z7 - z1) - (z4 - z2));
    fjzze = Real_t(.125) * ((z6 - z0) + (z5 - z3) + (z7 - z1) + (z4 - z2));

    /* compute cofactors */
    cjxxi = (fjyet * fjzze) - (fjzet * fjyze);
    cjxet = -(fjyxi * fjzze) + (fjzxi * fjyze);
    cjxze = (fjyxi * fjzet) - (fjzxi * fjyet);

    cjyxi = -(fjxet * fjzze) + (fjzet * fjxze);
    cjyet = (fjxxi * fjzze) - (fjzxi * fjxze);
    cjyze = -(fjxxi * fjzet) + (fjzxi * fjxet);

    cjzxi = (fjxet * fjyze) - (fjyet * fjxze);
    cjzet = -(fjxxi * fjyze) + (fjyxi * fjxze);
    cjzze = (fjxxi * fjyet) - (fjyxi * fjxet);

    /* calculate partials :
       this need only be done for l = 0,1,2,3   since , by symmetry ,
       (6,7,4,5) = - (0,1,2,3) .
    */
    b[0][0] = -cjxxi - cjxet - cjxze;
    b[0][1] = cjxxi - cjxet - cjxze;
    b[0][2] = cjxxi + cjxet - cjxze;
    b[0][3] = -cjxxi + cjxet - cjxze;
    b[0][4] = -b[0][2];
    b[0][5] = -b[0][3];
    b[0][6] = -b[0][0];
    b[0][7] = -b[0][1];

    b[1][0] = -cjyxi - cjyet - cjyze;
    b[1][1] = cjyxi - cjyet - cjyze;
    b[1][2] = cjyxi + cjyet - cjyze;
    b[1][3] = -cjyxi + cjyet - cjyze;
    b[1][4] = -b[1][2];
    b[1][5] = -b[1][3];
    b[1][6] = -b[1][0];
    b[1][7] = -b[1][1];

    b[2][0] = -cjzxi - cjzet - cjzze;
    b[2][1] = cjzxi - cjzet - cjzze;
    b[2][2] = cjzxi + cjzet - cjzze;
    b[2][3] = -cjzxi + cjzet - cjzze;
    b[2][4] = -b[2][2];
    b[2][5] = -b[2][3];
    b[2][6] = -b[2][0];
    b[2][7] = -b[2][1];

    /* calculate jacobian determinant (volume) */
    *volume = Real_t(8.) * (fjxet * cjxet + fjyet * cjyet + fjzet * cjzet);
}

int main(int argc, char *argv[])
{
    std::cout << "Testing CalcElemShapeFunctionDerivatives" << std::endl;

    Real_t x[8] = {0.0, 1.3, 2.3, 3.3, 4.3, 5.3, 6.3, 7.3};
    Real_t y[8] = {8.3, 9.3, 10.3, 11.3, 12.3, 13.3, 14.3, 15.3};
    Real_t z[8] = {16.3, 17.3, 18.3, 19.3, 20.3, 21.3, 22.3, 23.3};
    Real_t b[3][8];
    Real_t volume;

    CalcElemShapeFunctionDerivatives(x, y, z, b, &volume);

    std::cout << std::scientific;
    std::cout.precision(7);
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 8; ++j)
        {
            std::cout << "b[" << i << "][" << j << "] = " << b[i][j] << std::endl;
        }
    }
    std::cout << "Volume: " << volume << std::endl;

    return 0;
}