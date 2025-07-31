typedef float Real_t;

Real_t shifted_square(Real_t x, Real_t h)
{
    Real_t x_plus_eps = x + h;
    Real_t fx_x_h = x_plus_eps * x_plus_eps;
    return fx_x_h;
}