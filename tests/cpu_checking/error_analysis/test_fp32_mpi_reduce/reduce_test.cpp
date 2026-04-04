#include <mpi.h>
#include <cstdio>

__attribute__((noinline)) float sum_floats(const float *data, int n) {
    float s = 0.0f;
    for (int i = 0; i < n; i++) {
        s += data[i];
    }
    return s;
}

__attribute__((noinline)) double sum_doubles(const double *data, int n) {
    double s = 0.0;
    for (int i = 0; i < n; i++) {
        s += data[i];
    }
    return s;
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    const int N = 10;
    float vals_f[N];
    double vals_d[N];

    for (int i = 0; i < N; i++) {
        vals_f[i] = 0.1f * (i + 1);
        vals_d[i] = (double)vals_f[i];
    }

    float local_sum = sum_floats(vals_f, N);
    double local_sum_d = sum_doubles(vals_d, N);

    float reduced_sum = 0.0f;
    MPI_Reduce(&local_sum, &reduced_sum, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Use reduced_sum after reduce - error should be propagated
    float final_result = reduced_sum + 1.0f;
    double final_result_d = local_sum_d + 1.0;

    double total_error = final_result_d - (double)final_result;
    printf("FP64 total error: %.17e\n", total_error);

    MPI_Finalize();
    return 0;
}
