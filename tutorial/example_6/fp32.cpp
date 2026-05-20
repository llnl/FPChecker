// fp32.cpp — all computation in FP32 (instrumented by FPChecker)
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

struct Results {
  float root_small;
  float variance;
};

struct Coeffs {
  float a, b, c;
};

// --------------- variance: one-pass formula (numerically unstable) ----------
__attribute__((noinline))
static float variance_one_pass_fp32(const std::vector<float> &x) {
  float sum = 0.0f;
  float sumsq = 0.0f;

  for (size_t i = 0; i < x.size(); ++i) {
    sum += x[i];
    sumsq += x[i] * x[i];
  }

  const float n = static_cast<float>(x.size());
  const float mean = sum / n;
  return (sumsq / n) - (mean * mean);    // catastrophic cancellation
}

// --------------- main problem driver --------------------------------------
__attribute__((noinline))
static Results run_problem_fp32(int n, float amp, const Coeffs &coeff) {
  // --- data generation ---
  std::vector<float> x(static_cast<size_t>(n));
  for (int i = 0; i < n; ++i) {
    const float trend = 1.0e8f;
    const float wave = amp * sinf(0.01f * static_cast<float>(i));
    const float jitter = 0.25f * static_cast<float>(i % 9);
    x[static_cast<size_t>(i)] = trend + wave + jitter;
  }

  // --- quadratic small root (unstable form) ---
  const float a = coeff.a;
  const float b = coeff.b;
  const float c = coeff.c;
  const float disc = b * b - 4.0f * a * c;
  const float sqrt_d = sqrtf(disc);
  const float numer = -b + sqrt_d;                    // catastrophic cancellation
  const float root_small = numer / (2.0f * a);

  // --- variance (one-pass) ---
  const float variance = variance_one_pass_fp32(x);

  return {root_small, variance};
}

int main(int argc, char **argv) {
  const int    n   = (argc > 1) ? std::atoi(argv[1])            : 200000;
  const float  amp = (argc > 2) ? std::strtof(argv[2], nullptr) : 32.0f;

  Coeffs coeff;
  coeff.a = (argc > 3) ? std::strtof(argv[3], nullptr) : 1.0f;
  coeff.b = (argc > 4) ? std::strtof(argv[4], nullptr) : 1.0e8f;
  coeff.c = (argc > 5) ? std::strtof(argv[5], nullptr) : 1.0f;

  const Results r = run_problem_fp32(n, amp, coeff);

  std::cout << std::setprecision(10);
  std::cout << "mode=fp32\n";
  std::cout << "n=" << n << " amp=" << amp << "\n";
  std::cout << "root_small=" << r.root_small << "\n";
  std::cout << "variance="   << r.variance   << "\n";
  return 0;
}
