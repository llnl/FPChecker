#include <stdio.h>
#include <math.h>

//---------------------------------------------------------------------------------------
// Root of a polynomial by Newton's method

/*
 * Root of a polynomial by Newton's method
 *
 * With CADNA, one can see that 7 significant digits were lost (despite the apparent stability).
 * By using the symbolic debugger, one can see that, at the last iteration, the denominator is a no significant value (a computed zero) and that the last answer of the stopping criterion is not reliable.
 * CADNA allows to stop the algorithm when the subtraction x-y is no significant (there is no more information to compute at the next iteration).
 * In Newton's method, a division by a computed zero may suggest a double root. One can simplify the fraction.
 * When these two transformations are done, the code is stabilized and the results are obtained with the best accuraccy of the computer. The exact value of the root is X_sol = 3/7 = 0.428571428571428571....
 *
 * http://www-pequan.lip6.fr/cadna/Examples_Dir/ex5.php
 */
void newton()
{
//  std::cout << "Root of a polynomial by Newton's method" << '\n'
 //           << "One can see that 7 significant digits were lost (despite the apparent stability)." << std::endl;

  double eps = 1.0e-12;
  int nmax = 100;

  int i;
  double y = 0.5;
  double x = y + 2.0*eps;
  for (i = 1; (i <= nmax) && (abs(x-y) >= eps); i++)
  {
    x = y;
    y = x - (1.47*pow(x,3.0) + 1.19*pow(x,2.0) - 1.83*x + 0.45) / (4.41*pow(x,2.0) + 2.38*x - 1.83);
  }

  //std::cout << "x(" << i-1 << ") = " << x << std::endl;
  //std::cout << "x(" << i << ") = " << y << std::endl;
  //displayError(y, 0.428571428571428571);

  //std::cout << std::endl;
}

int main()
{
  newton();
}
