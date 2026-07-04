extern "C" __attribute__((noinline)) void overwrite_double(double *dst, double value)
{
    *dst = value;
    asm volatile("" : : "m"(*dst) : "memory");
}
