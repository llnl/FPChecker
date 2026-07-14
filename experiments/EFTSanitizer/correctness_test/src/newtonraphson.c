#include <stdio.h>
#include <stdlib.h>
extern void eftsan_print_error(double);
double f(double x) {
    return 3 * x - x * x - x * x * x + 16;
}

double df(double x) {
    return 3 - 2 * x - 3 * x * x;
}

void floatNewtonRaphson() {
	double oldX, x = 25.0f;
	double eps = 0.00001f;
	int maxStep = 100;
	int iter = 0;
	double d; 

	oldX = x;
	double diffX = df(x);
	if (fabs(diffX) != 0.0 && !isnan(diffX)) {
		x = x - f(x) / diffX;
	}
	else {
		printf("Unexpected slope\n");
		exit(1);
	}
	d = fabs(oldX - x);
	while (d > eps && iter < maxStep){
		oldX = x;
		double diffX = df(x);
		if (fabs(diffX) != 0.0 && !isnan(diffX)) {
			x = x - f(x) / diffX;
		}
		else {
			printf("Unexpected slope\n");
			exit(1);
		}
		iter ++;
		d = fabs(oldX - x);
	}
	eftsan_print_error(x);
	printf("%.50e\n", x);
}



int main() {
    floatNewtonRaphson();
    
}

