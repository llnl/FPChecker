// mixed.cpp — selective FP64 promotion guided by FPChecker rounding-error report
//
// Only the lines / regions flagged with HIGH relative error in the FP32 report
// are promoted to FP64.  Everything else stays FP32.
//
// Report-driven decisions (line numbers refer to fp32.cpp):
//
//   HIGH ERROR — promoted to FP64:
//     Line 49: disc = b*b - 4*a*c            (rel err ~3e-08)
//     Line 50: sqrt_d = sqrtf(disc)           (rel err ~1e-16)
//     Line 51: numer = -b + sqrt_d            (rel err  1.0 — total loss!)
//     Line 52: root_small = numer / (2*a)     (rel err  1.0 — propagated)
//       => Entire quadratic section promoted to FP64 + stable Vieta's form.
//          The cancellation in (-b + sqrt(disc)) loses ALL significant digits
//          when b ≈ sqrt(disc), which happens for b >> c.
//
//     Line 24: sum   += x[i]                  (rel err ~7e-04)
//     Line 25: sumsq += x[i]*x[i]            (rel err ~7e-04)
//     Line 29: mean  = sum / n                (rel err ~7e-04)
//     Line 30: (sumsq/n) - (mean*mean)        (rel err ~1e+10 — catastrophic!)
//       => Entire variance function promoted: FP64 accumulators + two-pass
//          algorithm to avoid the cancellation in E[x²]−E[x]².
//
//   LOW ERROR — kept as FP32:
//     Line 40: wave = amp * sinf(...)          (rel err ~1e-05)
//     Line 41: jitter = 0.25f * ...            (rel err  0)
//     Line 42: x[i] = trend + wave + jitter    (rel err ~2e-08)
//       => Data generation has negligible error; storage stays float.
//
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
  float a, b, c;
};

// --------------- variance: two-pass with FP64 accumulators -----------------
// Promoted because the one-pass formula showed rel. error ~1e+10 at the
// subtraction line (sumsq/n - mean*mean).  Two-pass avoids that cancellation,
// and FP64 accumulators prevent intermediate rounding from growing.
__attribute__((noinline))
static double variance_two_pass_mixed(const std::vector<float> &x) {
  // Pass 1: compute mean in FP64
  double sum = 0.0;
  for (size_t i = 0; i < x.size(); ++i) {
    sum += static_cast<double>(x[i]);
  }
  const double n    = static_cast<double>(x.size());
  const double mean = sum / n;

  // Pass 2: sum of squared deviations in FP64
  double sq_acc = 0.0;
  for (size_t i = 0; i < x.size(); ++i) {
    const double d = static_cast<double>(x[i]) - mean;
    sq_acc += d * d;
  }
  return sq_acc / n;
}

// --------------- main problem driver --------------------------------------
__attribute__((noinline))
static Results run_problem_mixed(int n, float amp, const Coeffs &coeff) {
  // --- data generation (FP32 — low error, kept as-is) ---
  std::vector<float> x(static_cast<size_t>(n));
  for (int i = 0; i < n; ++i) {
    const float trend = 1.0e8f;
    const float wave  = amp * std::sin(0.01f * static_cast<float>(i));
    const float jitter = 0.25f * static_cast<float>(i % 9);
    x[static_cast<size_t>(i)] = trend + wave + jitter;
  }

  // --- quadratic small root (promoted to FP64 + stable form) ---
  // The FP32 report showed catastrophic cancellation at (-b + sqrt(disc)),
  // producing root_small = 0.  We promote to FP64 and use Vieta's formula
  // (c / (a * large_root)) to avoid the subtraction of nearly-equal values.
  const double a = static_cast<double>(coeff.a);
  const double b = static_cast<double>(coeff.b);
  const double c = static_cast<double>(coeff.c);
  const double disc       = b * b - 4.0 * a * c;
  const double sqrt_disc  = std::sqrt(disc);
  const double large_root = (-b - sqrt_disc) / (2.0 * a);
  const double root_small = c / (a * large_root);

  // --- variance (promoted to FP64 two-pass) ---
  const double variance = variance_two_pass_mixed(x);

  return {root_small, variance};
}

int main(int argc, char **argv) {
  const int    n   = (argc > 1) ? std::atoi(argv[1])            : 200000;
  const float  amp = (argc > 2) ? std::strtof(argv[2], nullptr) : 32.0f;

  Coeffs coeff;
  coeff.a = (argc > 3) ? std::strtof(argv[3], nullptr) : 1.0f;
  coeff.b = (argc > 4) ? std::strtof(argv[4], nullptr) : 1.0e8f;
  coeff.c = (argc > 5) ? std::strtof(argv[5], nullptr) : 1.0f;

  const Results r = run_problem_mixed(n, amp, coeff);

  std::cout << std::setprecision(17);
  std::cout << "mode=mixed\n";
  std::cout << "n=" << n << " amp=" << amp << "\n";
  std::cout << "root_small=" << r.root_small << "\n";
  std::cout << "variance="   << r.variance   << "\n";
  return 0;
}
