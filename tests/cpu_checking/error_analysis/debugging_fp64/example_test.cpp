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
    int num_additions = 10000;

    float addend_f = 3348899234.88999922f;
    double addend_d = 3348899234.88999922;
    long double addend_ld = 3348899234.88999922L; 

    // float addend_f = 1.999f;
    // double addend_d = 1.999;
    // long double addend_ld = 1.999L; 
   
    float sum_f;
    double sum_d;
    long double sum_ld;

    compute_sum_float(num_additions, addend_f, &sum_f);
    compute_sum_double(num_additions, addend_d, &sum_d);
    compute_sum_long_double(num_additions, addend_ld, &sum_ld);

    float avg_f = sum_f / num_additions;
    double avg_d = sum_d / num_additions;
    long double avg_ld = sum_ld / num_additions;

    // printf("--- SUM RESULTS ---\n");
    // printf("Summation(Float):        %.10f\n", sum_f);
    // printf("Summation(Double):       %.17f\n", sum_d);
    // printf("Summation(Long Double):  %.21Lf\n", sum_ld);

    printf("\n--- SUM DIFFERENCES ---\n");
    // printf("Single to double precision: %.17e\n", sum_d - (double)sum_f);
    printf("Double to long double precision: %.21Le\n", (long double)sum_d - sum_ld);
    // printf("Result in Long Double: %.21Lf\n",sum_ld);

    // printf("\n--- AVERAGE RESULTS ---\n");
    // printf("Average(Float):        %.10f\n", avg_f);
    // printf("Average(Double):       %.17f\n", avg_d);
    // printf("Average(Long Double):  %.21Lf\n", avg_ld);

    printf("\n--- AVERAGE DIFFERENCES ---\n");
    // printf("Single to double precision: %.17e\n", avg_d - (double)avg_f);
    printf("Double to long double precision: %.21Le\n", avg_d - avg_ld);
    printf("Long Doublee:  %.21Le\n", avg_ld);

    return 0;
}