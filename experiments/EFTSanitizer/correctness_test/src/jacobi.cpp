//
// Created by chevalier on 08/08/18.
//

#include <iostream>
#include <array>
#include <cmath>
#include <math.h>

//---------------------------------------------------------------------------------------
// Jacobi's method

int nrand = 23;
float random1()
{
  nrand = (nrand*5363 + 143) % 1387;
  return static_cast<float>(2.0*nrand/1387.0 - 1.0);
}

/*
 * Jacobi's method
 *
 * With the classical arithmetic, the stopping criterion is satisfied at the 1000-th iteration. But, from the 30th iteration there is no numerical improvement of the results.
 * A kind of "trampling" appears. the value of eps is too small.
 * With CADNA, the program is stopped when, for all components, difference between two iterations becomes no significant. It happens at the 30-th iteration.
 * For a same numerical quality, CADNA has divided the number of iteration by a factor 30.
 *
 * http://www-pequan.lip6.fr/cadna/Examples_Dir/ex7.php
 */
void jacobi()
{
  std::cout << "Jacobi's method" << std::endl;

  float eps = 1e-4f;
  int ndim = 20;
  int niter = 1000;
  std::array<std::array<float, 20>, 20> a;
  std::array<float, 20> b;
  std::array<float, 20> x;
  std::array<float, 20> y;
  std::array<float, 20> xsol;

  xsol[0] = 1.7f;
  xsol[1] = -4746.89f;
  xsol[2] = 50.23f;
  xsol[3] = -245.32f;
  xsol[4] = 4778.29f;
  xsol[5] = -75.73f;
  xsol[6] = 3495.43f;
  xsol[7] = 4.35f;
  xsol[8] = 452.98f;
  xsol[9] = -2.76f;
  xsol[10] = 8239.24f;
  xsol[11] = 3.46f;
  xsol[12] = 1000.0f;
  xsol[13] = -5.0f;
  xsol[14] = 3642.4f;
  xsol[15] = 735.36f;
  xsol[16] = 1.7f;
  xsol[17] = -2349.17f;
  xsol[18] = -8247.52f;
  xsol[19] = 9843.57f;

  for (int i = 0; i < ndim; i++)
  {
    for (int j = 0; j < ndim; j++)
    {
      a[i][j] = random1();
    }
    a[i][i] = a[i][i] + 4.9213648f;
  }
  for (int i = 0; i < ndim; i++)
  {
    float aux = 0.0f;
    for (int j = 0; j < ndim; j++)
    {
      aux = aux + a[i][j] * xsol[j];
    }
    b[i] = aux;
    y[i] = 10.0f;
  }

  int i;
  float anorm = 1.0f + eps;
  for (i = 0; (i < niter) && (anorm >= eps); i++)
  {
    anorm = 0.0f;
    for (int j = 0; j < ndim; j++)
    {
      x[j] = y[j];
    }
    for (int j = 0; j < ndim; j++)
    {
      float aux = b[j];
      for (int k = 0; k < ndim; k++)
      {
        if (k != j)
        {
          aux = aux - a[j][k] * x[k];
        }
      }
      y[j] = aux / a[j][j];
      if (std::abs(x[j] - y[j]) > anorm)
      {
        anorm = std::abs(x[j] - y[j]);
      }
    }
  }

  std::cout << "niter = " << i << std::endl;
  for (int i = 0; i < ndim; i++)
  {
    float aux = -b[i];
    for (int j = 0; j < ndim; j++)
    {
      aux = aux + a[i][j] * y[j];
    }
    std::cout << "x_sol(" << i+1 << ") = " << y[i] << " (true value : " << (double) xsol[i] << "), residue(" << i+1 << ") = " << aux << std::endl;
    //displayError(y[i], (double) xsol[i]);
  }

  std::cout << std::endl;
}

int main()
{
  jacobi();
}
