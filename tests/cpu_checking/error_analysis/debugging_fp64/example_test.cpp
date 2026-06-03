#include <stdio.h>


void compute_sum_float(int num_additions, float addend, float *sum)
{
    *sum = 0.0f;
    for (int i = 0; i < num_additions; i++)
    {
        *sum += addend;
    }
}

void compute_sum_double(int num_additions, double addend, double *sum)
{
    *sum = 0.0;
    for (int i = 0; i < num_additions; i++)
    {
        *sum += addend;
    }
}

void compute_sum_long_double(int num_additions, long double addend, long double *sum)
{
    *sum = 0.0L;
    for (int i = 0; i < num_additions; i++)
    {
        *sum += addend;
    }
}

int main(void)
{
    int num_additions = 100000;

    double addend_d = 0.1;
    long double addend_ld = 0.1L; 
    
    double sum_d;
    long double sum_ld;

    // compute_sum_float(num_additions, addend_f, &sum_f);
    compute_sum_double(num_additions, addend_d, &sum_d);
    compute_sum_long_double(num_additions, addend_ld, &sum_ld);

    double avg_d = sum_d / num_additions;
    long double avg_ld = sum_ld / num_additions;


    printf("\n--- SUM DIFFERENCES ---\n");
    // printf("Single to double precision: %.17e\n", sum_d - (double)sum_f);
    printf("Double to long double precision: %.21Le\n", (long double)sum_d - sum_ld);
    // printf("Result in Long Double: %.21Lf\n",sum_ld);

    printf("\n--- AVERAGE DIFFERENCES ---\n");

    printf("Double to long double precision: %.21Le\n", avg_d - avg_ld);


    return 0;
}