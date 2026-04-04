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

    float bcast_val = sum_floats(vals_f, N);
    double bcast_val_d = sum_doubles(vals_d, N);

    MPI_Bcast(&bcast_val, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);

    // Use bcast_val after broadcast - error should be preserved
    float final_result = bcast_val + 1.0f;
    double final_result_d = bcast_val_d + 1.0;

    double total_error = final_result_d - (double)final_result;
    printf("FP64 total error: %.17e\n", total_error);

    MPI_Finalize();
    return 0;
}
