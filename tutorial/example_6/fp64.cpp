// fp64.cpp — full FP64 reference (ground truth for accuracy comparison)
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

struct Results {
  double root_small;
  double variance;
};

struct Coeffs {
  double a, b, c;
};

// --------------- variance: two-pass in FP64 --------------------------------
__attribute__((noinline))
static double variance_two_pass_fp64(const std::vector<double> &x) {
  double sum = 0.0;
  for (size_t i = 0; i < x.size(); ++i) {
    sum += x[i];
  }
  const double n    = static_cast<double>(x.size());
  const double mean = sum / n;

  double sq_acc = 0.0;
  for (size_t i = 0; i < x.size(); ++i) {
    const double d = x[i] - mean;
    sq_acc += d * d;
  }
  return sq_acc / n;
}

// --------------- main problem driver --------------------------------------
__attribute__((noinline))
static Results run_problem_fp64(int n, double amp, const Coeffs &coeff) {
  std::vector<double> x(static_cast<size_t>(n));
  for (int i = 0; i < n; ++i) {
    const double trend  = 1.0e8;
    const double wave   = amp * std::sin(0.01 * static_cast<double>(i));
    const double jitter = 0.25 * static_cast<double>(i % 9);
    x[static_cast<size_t>(i)] = trend + wave + jitter;
  }

  // stable quadratic via Vieta's formula
  const double a = coeff.a;
  const double b = coeff.b;
  const double c = coeff.c;
  const double disc       = b * b - 4.0 * a * c;
  const double sqrt_disc  = std::sqrt(disc);
  const double large_root = (-b - sqrt_disc) / (2.0 * a);
  const double root_small = c / (a * large_root);

  const double variance = variance_two_pass_fp64(x);

  return {root_small, variance};
}

int main(int argc, char **argv) {
  const int    n   = (argc > 1) ? std::atoi(argv[1])            : 200000;
  const double amp = (argc > 2) ? std::strtod(argv[2], nullptr) : 32.0;

  Coeffs coeff;
  coeff.a = (argc > 3) ? std::strtod(argv[3], nullptr) : 1.0;
  coeff.b = (argc > 4) ? std::strtod(argv[4], nullptr) : 1.0e8;
  coeff.c = (argc > 5) ? std::strtod(argv[5], nullptr) : 1.0;

  const Results r = run_problem_fp64(n, amp, coeff);

  std::cout << std::setprecision(17);
  std::cout << "mode=fp64\n";
  std::cout << "n=" << n << " amp=" << amp << "\n";
  std::cout << "root_small=" << r.root_small << "\n";
  std::cout << "variance="   << r.variance   << "\n";
  return 0;
}
