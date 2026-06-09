#include <mpi.h>
#include <cstdio>

__attribute__((noinline)) double sum_floats(const double *data, int n) {
    double s = 0.0;
    for (int i = 0; i < n; i++) {
        s += data[i];
    }
    return s;
}

__attribute__((noinline)) long double sum_doubles(const long double *data, int n) {
    long double s = 0.0;
    for (int i = 0; i < n; i++) {
        s += data[i];
    }
    return s;
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    const int N = 10;
    double vals_f[N];
    long double vals_d[N];

    for (int i = 0; i < N; i++) {
        vals_f[i] = 0.1 * (i + 1);
        vals_d[i] = (long double)vals_f[i];
    }

    double local_sum = sum_floats(vals_f, N);
    long double local_sum_d = sum_doubles(vals_d, N);

    double reduced_sum = 0.0;
    MPI_Reduce(&local_sum, &reduced_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    // Use reduced_sum after reduce - error should be propagated
    double final_result = reduced_sum + 1.0;
    long double final_result_d = local_sum_d + 1.0;

    long double total_error = final_result_d - (long double)final_result;
    printf("FP64 total error: %.17Le\n", total_error);

    MPI_Finalize();
    return 0;
}
