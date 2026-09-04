#!/bin/bash
# build_instrumentation.sh -- build the nsan branch-flip pass plugin and the
# runtime shim, then self-test on a program with a known flip.
#
#   ./build_instrumentation.sh          # build + test
#   ./build_instrumentation.sh --test   # test only
#
# Run with nsan_env active. Produces plugin/libNsanBFSites.so (load with
# -fpass-plugin=) and runtime/libnsan_bf.a (link with $NSAN_WRAP_FLAGS).

set -uo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
TEST_ONLY=0
[ "${1:-}" = "--test" ] && TEST_ONLY=1

hr()   { printf '\n=== %s ===\n' "$1"; }
ok()   { printf '  [ ok ] %s\n' "$1"; }
warn() { printf '  [warn] %s\n' "$1"; }
die()  { printf '  [FAIL] %s\n' "$1"; exit 1; }

command -v clang >/dev/null || die "clang not on PATH -- conda activate nsan_env"
command -v llvm-config >/dev/null || die "llvm-config not found"

if [ "$TEST_ONLY" = 0 ]; then
hr "1. pass plugin"
  # -fno-rtti: LLVM is built without RTTI.
  clang++ -fPIC -shared -fno-rtti -O2 -std=c++17 \
    $(llvm-config --cxxflags | sed 's/-fno-exceptions//') \
    -I"$(llvm-config --includedir)" \
    "$HERE/plugin/NsanBFSites.cpp" \
    -o "$HERE/plugin/libNsanBFSites.so" 2>"$HERE/plugin/build.log" \
    || { sed 's/^/         /' "$HERE/plugin/build.log" | head -25; \
         die "plugin build failed (full log: plugin/build.log)"; }
  ok "plugin/libNsanBFSites.so"

hr "2. runtime shim"
  clang -fPIC -O2 -c "$HERE/runtime/nsan_bf_runtime.c" \
    -o "$HERE/runtime/nsan_bf_runtime.o" 2>"$HERE/runtime/build.log" \
    || { sed 's/^/         /' "$HERE/runtime/build.log" | head -25; \
         die "runtime build failed"; }
  ar rcs "$HERE/runtime/libnsan_bf.a" "$HERE/runtime/nsan_bf_runtime.o" \
    || die "ar failed"
  ok "runtime/libnsan_bf.a"
fi

hr "3. self-test"
# (a+b)-a with a=1e8, b=1: 0 in float, 1 in the double shadow.
mkdir -p "$HERE/selftest" && cd "$HERE/selftest"
cat > bf_selftest.c <<'EOF'
#include <stdio.h>
__attribute__((noinline)) float cancel(float a, float b) { return (a + b) - a; }
int main(void) {
  float d = cancel(1e8f, 1.0f);
  if (d > 0.5f) printf("gt\n"); else printf("le\n");   /* the flip site */
  for (int i = 0; i < 3; ++i)                          /* 3 executions, 0 flips */
    if ((float)i > 100.0f) printf("never\n");
  printf("d = %.9g\n", d);
  return 0;
}
EOF

WRAP="-Wl,--wrap=__nsan_fcmp_fail_float_d \
-Wl,--wrap=__nsan_fcmp_fail_double_q \
-Wl,--wrap=__nsan_fcmp_fail_double_l \
-Wl,--wrap=__nsan_fcmp_fail_longdouble_q"

NSAN_BF_OPT_LEVEL=O0 clang -g -O0 -fsanitize=numerical \
  -fpass-plugin="$HERE/plugin/libNsanBFSites.so" \
  -mllvm -nsan-truncate-fcmp-eq=0 \
  bf_selftest.c "$HERE/runtime/libnsan_bf.a" $WRAP \
  -o bf_selftest 2>&1 | tee compile.log | grep -E '^\[NSanBF\]' \
  || warn "no [NSanBF] banner -- the plugin did not run"

[ -f bf_selftest ] || die "self-test binary not produced; see selftest/compile.log"

echo "  --- manifest ---"
cat bf_selftest.c.nsansites 2>/dev/null | sed 's/^/  /' \
  || warn "no .nsansites manifest written"

echo "  --- run ---"
ulimit -s 65536
NSAN_BF_LOG=events.log \
NSAN_OPTIONS=halt_on_error=0:resume_after_warning=0:disable_warnings=1 \
  ./bf_selftest
echo "  --- events ---"
sed 's/^/  /' events.log

hr "what to check"
cat <<'EOF'
  1. The manifest lists 2 branch sites: the `d > 0.5f` at line 5 and the
     `(float)i > 100.0f` at line 7. Both with n_fcmp 1.
  2. Exactly one #NSAN_EVENT, at the line-5 site, with k=0.
  3. #NSAN_SITE for the line-7 site shows 3 executions and 0 flagged --
     that is the per-site total that gives the scorer its denominator.
  4. #NSAN_TOTALS shows unticked=0. Anything else means the plugin and the
     nsan pass disagree about which fcmps are instrumented, and no run from
     this build should be scored.
EOF
