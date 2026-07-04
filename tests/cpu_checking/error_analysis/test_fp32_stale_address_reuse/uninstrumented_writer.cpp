extern "C" __attribute__((noinline)) void overwrite_float(float *dst, float value)
{
    *dst = value;
    asm volatile("" : : "m"(*dst) : "memory");
}
