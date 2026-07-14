#include <stdio.h>
#include <math.h>


//---------------------------------------------------------------------------------------
// SECOND ORDER EQUATION

/*
 * The standard floating point arithmetic cannot detect that d=0. The wrong branching is performed and the result is false.
 *
 * http://www-pequan.lip6.fr/cadna/Examples_Dir/ex2.php
 */
void secondOrder()
{
  double a = 0.3;
  double b = -2.1;
  double c = 3.675;

  if (a == 0.0)
  {
    if (b == 0.0)
    {
      if (c == 0.0)
      {
        printf("Every complex value is solution.\n");
      }
      else
      {
        printf("There is no solution\n");
      }
    }
    else
    {
      double x1 = - c/b;
      printf("The equation is degenerated. There is one real64 solution:%e\n", x1);
    }
  }
  else
  {
    b = b/a;
    c = c/a;
    double d = b*b - 4.0*c;
    printf("d = %e", d);
    //displayError(d, 0);

    if (d == 0.0)
    {
      double x1 = -b*0.5;
      printf("Discriminant is zero. The double solution is:%e\n", x1);
      //displayError(x1, 3.5);
    }
    else if (d > 0.0)
    {
      double x1 = ( - b - sqrt(d))*0.5;
      double x2 = ( - b + sqrt(d))*0.5;
      printf("There are two real64 solutions. x1 = %e x2 = %e \n", x1, x2);
      //displayError(x1, 3.5);
      //displayError(x2, 3.5);
    }
    else
    {
      double x1 = - b*0.5;
      double x2 = sqrt(-d)*0.5;
      printf("x1:%e x2:%e\n", x1, x2);
//      std::cout << "There are two complex solutions."
 //               << "z1 = " << x1 << " + i * " << x2 << ' '
  //              << "z2 = " << x1 << " + i * " << -x2
   //             << std::endl;
      //displayError(x1, 3.5);
    }
  }

//  std::cout << std::endl;
}

int main() {
  secondOrder();
}
