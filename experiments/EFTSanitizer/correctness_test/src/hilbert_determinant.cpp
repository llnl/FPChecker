#include <cmath>
#include <iostream>
#include <array>

//---------------------------------------------------------------------------------------
// DETERMINANT OF A HILBERT MATRIX

/*
 * determinant of Hilbert's matrix
 *
 * The gradual loss of accuracy is pointed out by CADNA.
 * One can see that the value of the determinant is significant even if it is very "small".
 * This shows how difficult it is to judge the numerical quality of a computed result by its magnitude.
 *
 * http://www-pequan.lip6.fr/cadna/Examples_Dir/ex3.php
 */
void hilbert()
{
  std::cout << "The determinant of Hilbert's matrix" << '\n'
            << "The gradual loss of accuracy is pointed out." << '\n'
            << "One can see that the value of the determinant is significant even if it is very \"small\"." << std::endl;

  std::array<std::array<double, 11>, 11> amat;
  for (int i = 1; i <= 11; i++)
  {
    for (int j = 1; j <= 11; j++)
    {
      amat[i-1][j-1] = 1.0 / (i+j-1);
    }
  }
  double det = 1.0;

  int i;
  for (i = 1; i <= 10; i++)
  {
    double pivot = amat[i-1][i-1];
    std::cout << "Pivot double " << i << " = " << pivot << std::endl;
    det = det * pivot;

    double aux = 1.0 / pivot;
    for (int j = i+1; j <= 11; j++)
    {
      amat[i-1][j-1] = amat[i-1][j-1] * aux;
    }

    for (int j = i+1; j <= 11; j++)
    {
      aux = amat[j-1][i-1];
      for (int k = i+1; k<= 11; k++)
      {
        amat[j-1][k-1] = amat[j-1][k-1] - aux*amat[i-1][k-1];
      }
    }
  }

  double pivot = amat[i-1][i-1];
  det = det * pivot;
  std::cout << "Pivot double " << i << " = " << pivot << std::endl;
  std::cout << "Determinant = " << det << std::endl;

  std::cout << std::endl;
}

int main()
{
  hilbert();
  return EXIT_SUCCESS;
}
