#include <stdio.h>
extern void eftsan_print_error(double);
// calculating dot product. The intermediate result has less precision bits in posit than float.
// Altered from End of error example.
int main( int argc, const char* argv[] )
{
  float fa[4] = {1.000010223508783104E+18, 1.00002781569482752E+17, -1.000010223508783104E+18, -1.00002781569482752E+17};
  float fb[4] = {1.000010223508783104E+18, 1.00002781569482752E+17, 1.000010223508783104E+18, -1.00002781569482752E+17};
    float fresult = 0.0;
    for (int i = 0; i < 4; i++) {
        fresult += fa[i] * fb[i];
    }
    eftsan_print_error(fresult);
    printf("float    result: %.50e\n", fresult);
    return 0;
}
