#!/bin/bash
# nsan_setup.sh -- build the nsan runtime into the active env and verify it.
#
#   ./nsan_setup.sh                 # detect, fetch, build if needed, verify
#   ./nsan_setup.sh --force-build   # rebuild the runtime even if present
#   ./nsan_setup.sh --no-build      # detect + verify only
#   ./nsan_setup.sh --verify-only   # skip fetch/build, rerun the toy
#   ./nsan_setup.sh -j 32           # build parallelism (default 16)
#
# Run with nsan_env active. Sparse-clones llvm-project at llvmorg-19.1.7,
# builds only compiler-rt/nsan standalone against the installed LLVM, installs
# it into the clang resource dir, runs a toy with one known flip, and writes
# nsan_setup/nsan_env.sh with the settled flags. Idempotent.

set -uo pipefail

#--------------------------------------------------------------------------
# Configuration
#--------------------------------------------------------------------------
LLVM_TAG=llvmorg-19.1.7
ROOT="${NSAN_ROOT:-$PWD/nsan_setup}"
SRC="${NSAN_SRC:-$ROOT/llvm19-src}"
JOBS=16
DO_FETCH=1
DO_BUILD=1
FORCE_BUILD=0
DO_VERIFY=1

while [ $# -gt 0 ]; do
  case "$1" in
    --force-build) FORCE_BUILD=1 ;;
    --no-build)    DO_BUILD=0 ;;
    --verify-only) DO_FETCH=0; DO_BUILD=0 ;;
    -j)            shift; JOBS="$1" ;;
    -j*)           JOBS="${1#-j}" ;;
    --root)        shift; ROOT="$1" ;;
    --src)         shift; SRC="$1" ;;
    -h|--help)     sed -n '2,13p' "$0"; exit 0 ;;
    *) echo "unknown argument: $1"; exit 2 ;;
  esac
  shift
done

mkdir -p "$ROOT" || exit 1
LOG="$ROOT/setup.log"
: > "$LOG"

#--------------------------------------------------------------------------
# Output helpers
#--------------------------------------------------------------------------
hr()   { printf '\n\033[1m=== %s ===\033[0m\n' "$1" | tee -a "$LOG"; }
ok()   { printf '  [ ok ] %s\n' "$1" | tee -a "$LOG"; }
warn() { printf '  [warn] %s\n' "$1" | tee -a "$LOG"; }
bad()  { printf '  [FAIL] %s\n' "$1" | tee -a "$LOG"; }
info() { printf '         %s\n' "$1" | tee -a "$LOG"; }
die()  { bad "$1"; printf '\n  full log: %s\n' "$LOG"; exit 1; }

#==========================================================================
hr "1. toolchain"
#==========================================================================
command -v clang    >/dev/null || die "clang not on PATH -- activate nsan_env"
command -v clang++  >/dev/null || die "clang++ not on PATH"

CLANG=$(readlink -f "$(command -v clang)")
CLANGXX=$(readlink -f "$(command -v clang++)")
PREFIX=$(cd "$(dirname "$CLANG")/.." && pwd)
CLANG_VER=$(clang --version | sed -n '1s/.*version \([0-9.]*\).*/\1/p')

info "clang       $CLANG"
info "version     $CLANG_VER"
info "prefix      $PREFIX"

case "$CLANG_VER" in
  19.*) ok "LLVM 19" ;;
  *)    die "need LLVM 19.x for -fsanitize=numerical; found $CLANG_VER" ;;
esac

if command -v llvm-config >/dev/null; then
  LLVM_CMAKE_DIR=$(llvm-config --cmakedir 2>/dev/null)
  LLVM_CONFIG=$(command -v llvm-config)
  info "llvm-config $LLVM_CONFIG"
  info "cmake dir   ${LLVM_CMAKE_DIR:-<none>}"
else
  LLVM_CMAKE_DIR=""; LLVM_CONFIG=""
  warn "llvm-config not found -- needed to build the runtime"
fi

ARCH=$(uname -m)
[ "$ARCH" = "x86_64" ] || warn "nsan in LLVM 19 supports x86_64 only; this is $ARCH"

RESDIR=$(clang -print-resource-dir 2>/dev/null)
info "resource    $RESDIR"

#==========================================================================
hr "2. is the nsan runtime present?"
#==========================================================================
find_nsan_rt() { find "$RESDIR/lib" -name 'libclang_rt.nsan*' 2>/dev/null | sort; }

NSAN_RT=$(find_nsan_rt)
if [ -n "$NSAN_RT" ]; then
  ok "runtime present"
  echo "$NSAN_RT" | sed 's/^/         /' | tee -a "$LOG"
  NEED_BUILD=0
else
  warn "no libclang_rt.nsan* under $RESDIR/lib"
  info "other sanitizer runtimes that ARE present:"
  find "$RESDIR/lib" -name 'libclang_rt.*san*' 2>/dev/null \
    | sed 's|.*/|           |' | sort -u | head | tee -a "$LOG"
  NEED_BUILD=1
fi
[ "$FORCE_BUILD" = 1 ] && NEED_BUILD=1

RT_LIBDIR=$(dirname "$(find "$RESDIR/lib" -name 'libclang_rt.asan*' -o \
                        -name 'libclang_rt.ubsan*' 2>/dev/null | head -1)" \
            2>/dev/null)
[ -z "$RT_LIBDIR" ] && RT_LIBDIR="$RESDIR/lib/linux"
info "runtime libdir: $RT_LIBDIR"

#==========================================================================
hr "3. sources"
#==========================================================================
if [ "$DO_FETCH" = 1 ]; then
  if [ -d "$SRC/.git" ]; then
    HAVE_TAG=$(git -C "$SRC" describe --tags 2>/dev/null || echo unknown)
    ok "already cloned at $SRC ($HAVE_TAG)"
  else
    info "sparse-cloning $LLVM_TAG into $SRC (blobless; a few minutes)"
    mkdir -p "$(dirname "$SRC")"
    git clone --filter=blob:none --sparse --depth 1 --branch "$LLVM_TAG" \
      https://github.com/llvm/llvm-project.git "$SRC" >>"$LOG" 2>&1 \
      || die "clone failed -- see $LOG (proxy/network?)"
    ok "cloned"
  fi

  git -C "$SRC" sparse-checkout set \
      compiler-rt \
      cmake \
      llvm/cmake \
      llvm/lib/Transforms/Instrumentation \
      llvm/include/llvm/Transforms/Instrumentation \
      clang/lib/CodeGen >>"$LOG" 2>&1 || die "sparse-checkout failed"

  PASS_SRC="$SRC/llvm/lib/Transforms/Instrumentation/NumericalStabilitySanitizer.cpp"
  [ -f "$PASS_SRC" ] || die "pass source missing after checkout"
  ok "pass source: $(wc -l < "$PASS_SRC") lines"
  ok "runtime source: $(wc -l < "$SRC/compiler-rt/lib/nsan/nsan.cpp") lines"
  info "tree size: $(du -sh "$SRC" | cut -f1)"
else
  info "skipped (--verify-only)"
fi

#==========================================================================
hr "4. build the nsan runtime"
#==========================================================================
if [ "$NEED_BUILD" = 1 ] && [ "$DO_BUILD" = 1 ]; then
  [ -d "$SRC/compiler-rt" ] || die "need sources to build; drop --verify-only"
  [ -n "$LLVM_CMAKE_DIR" ] || die "llvm-config required to build compiler-rt"
  command -v cmake >/dev/null || die "cmake not found (conda install cmake ninja)"

  BUILD="$ROOT/compiler-rt-build"
  if command -v ninja >/dev/null; then GEN="Ninja"; BUILDCMD="ninja -j$JOBS"
  else                                 GEN="Unix Makefiles"; BUILDCMD="make -j$JOBS"; fi

  TRIPLE=$(clang -print-target-triple 2>/dev/null)
  [ -n "$TRIPLE" ] || TRIPLE="$(uname -m)-unknown-linux-gnu"
  info "target triple: $TRIPLE"
  info "generator: $GEN, -j$JOBS"
  info "building compiler-rt/nsan only (~15-25 min)"

  cmake -G "$GEN" -S "$SRC/compiler-rt" -B "$BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCOMPILER_RT_STANDALONE_BUILD=ON \
    -DLLVM_CMAKE_DIR="$LLVM_CMAKE_DIR" \
    -DLLVM_CONFIG_PATH="$LLVM_CONFIG" \
    -DCMAKE_C_COMPILER="$CLANG" \
    -DCMAKE_CXX_COMPILER="$CLANGXX" \
    -DCMAKE_C_COMPILER_TARGET="$TRIPLE" \
    -DCMAKE_CXX_COMPILER_TARGET="$TRIPLE" \
    -DCMAKE_ASM_COMPILER_TARGET="$TRIPLE" \
    -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON \
    -DCOMPILER_RT_BUILD_BUILTINS=OFF \
    -DCOMPILER_RT_BUILD_SANITIZERS=ON \
    -DCOMPILER_RT_BUILD_XRAY=OFF \
    -DCOMPILER_RT_BUILD_LIBFUZZER=OFF \
    -DCOMPILER_RT_BUILD_PROFILE=OFF \
    -DCOMPILER_RT_BUILD_MEMPROF=OFF \
    -DCOMPILER_RT_BUILD_ORC=OFF \
    -DCOMPILER_RT_BUILD_CTX_PROFILE=OFF \
    -DCOMPILER_RT_INCLUDE_TESTS=OFF \
    >>"$LOG" 2>&1 || die "cmake configure failed -- tail of $LOG:
$(tail -25 "$LOG")"
  ok "configured"

  ( cd "$BUILD" && $BUILDCMD nsan ) >>"$LOG" 2>&1 \
    || die "build failed -- tail of $LOG:
$(tail -30 "$LOG")"
  ok "built"

  BUILT=$(find "$BUILD" -name 'libclang_rt.nsan*' -o -name 'nsan.syms' 2>/dev/null)
  [ -n "$BUILT" ] || die "build reported success but produced no nsan artifacts"
  echo "$BUILT" | sed 's/^/         built: /' | tee -a "$LOG"

  if [ -w "$RT_LIBDIR" ]; then
    mkdir -p "$RT_LIBDIR"
    for f in $BUILT; do cp -v "$f" "$RT_LIBDIR/" >>"$LOG" 2>&1; done
    ok "installed into $RT_LIBDIR"
  else
    bad "$RT_LIBDIR is not writable"
    info "copy manually, or add to every compile line:"
    info "  -resource-dir=<a writable copy of $RESDIR>"
    exit 1
  fi

  NSAN_RT=$(find_nsan_rt)
  [ -n "$NSAN_RT" ] || die "install did not land in the resource dir"
  ok "runtime now present"
elif [ "$NEED_BUILD" = 1 ]; then
  warn "runtime missing and building disabled (--no-build); verification will fail"
else
  info "nothing to build"
fi

#==========================================================================
hr "5. toy with a known flip"
#==========================================================================
# 0.1f accumulated 100 times: float gives 10.000002 (> 10 true), the double
# shadow gives 9.99999999 (false). One FP-controlled branch, one flip.
cat > "$ROOT/bf_toy.c" <<'EOF'
#include <stdio.h>

__attribute__((noinline)) float naive(int n) {
  float s = 0.0f;
  for (int i = 0; i < n; ++i) s += 0.1f;
  return s;
}

__attribute__((noinline)) int eq_probe(float x) {
  return x == 10.0f;          /* exercises -nsan-truncate-fcmp-eq */
}

int main(void) {
  float s = naive(100);
  int flipped = 0;
  if (s > 10.0f) { flipped = 1; printf("branch: gt\n"); }
  else           {              printf("branch: le\n"); }
  printf("eq_probe: %d\n", eq_probe(s));
  printf("s = %.9g  flipped=%d\n", s, flipped);
  return 0;
}
EOF

run_case() {                 # name, extra -mllvm flags
  local name="$1"; shift
  local out="$ROOT/toy_$name"
  if ! clang -g -O0 -fsanitize=numerical "$@" \
        "$ROOT/bf_toy.c" -o "$out" 2>"$ROOT/toy_$name.build.log"; then
    bad "$name: compile/link failed"
    sed 's/^/           /' "$ROOT/toy_$name.build.log" | head -6 | tee -a "$LOG"
    return 1
  fi
  NSAN_OPTIONS=halt_on_error=0:check_cmp=1 "$out" \
    >"$ROOT/toy_$name.out" 2>"$ROOT/toy_$name.err"
  local n
  n=$(grep -c 'NumericalStabilitySanitizer.*comparison' "$ROOT/toy_$name.err")
  printf '  [ ok ] %-28s comparison warnings: %s\n' "$name" "$n" | tee -a "$LOG"
  return 0
}

if [ "$DO_VERIFY" = 1 ]; then
  run_case "dqq_trunc_on"  -mllvm -nsan-shadow-type-mapping=dqq
  run_case "dqq_trunc_off" -mllvm -nsan-shadow-type-mapping=dqq \
                           -mllvm -nsan-truncate-fcmp-eq=0
  run_case "dlq_trunc_off" -mllvm -nsan-shadow-type-mapping=dlq \
                           -mllvm -nsan-truncate-fcmp-eq=0

  info ""
  info "expected: dqq_trunc_on has ONE warning (the > site);"
  info "          *_trunc_off has TWO (the > site and the == probe)."
  info "          If dqq_trunc_on is 0, the pass did not instrument."
  info ""
  info "first warning verbatim:"
  sed -n '1,12p' "$ROOT/toy_dqq_trunc_off.err" | sed 's/^/           /' | tee -a "$LOG"

fi

#==========================================================================
hr "6. settled flags"
#==========================================================================
cat > "$ROOT/nsan_env.sh" <<EOF
# nsan_env.sh -- generated by nsan_setup.sh on $(date -Iseconds)

export NSAN_CLANG="$CLANG"
export NSAN_CLANGXX="$CLANGXX"
export NSAN_RESOURCE_DIR="$RESDIR"
export NSAN_LLVM_SRC="$SRC"

# Shadow mapping, one id per {float,double,long double}: d=double l=x86_fp80
# q=fp128. dlq shadows double as x86_fp80, matching the fp64-vs-ld oracle.
export NSAN_MAP_FP32="-mllvm -nsan-shadow-type-mapping=dqq"
export NSAN_MAP_FP64="-mllvm -nsan-shadow-type-mapping=dlq"

# -nsan-truncate-fcmp-eq defaults on and suppresses == / != flips.
export NSAN_EQ_MEASURE="-mllvm -nsan-truncate-fcmp-eq=0"
export NSAN_EQ_DEFAULT=""

# Count loads that silently reset the shadow to the native value.
export NSAN_CHECK_LOADS="-mllvm -nsan-check-loads=1"

export NSAN_CFLAGS_COMMON="-g -O0 -fsanitize=numerical"

# The fail hook is wrapped by libnsan_bf, so the built-in reporter is off.
export NSAN_OPTIONS_MEASURE="halt_on_error=0:disable_warnings=1"
export NSAN_OPTIONS_DEBUG="halt_on_error=0:check_cmp=1:disable_warnings=0"

export NSAN_WRAP_FLAGS="-Wl,--wrap=__nsan_fcmp_fail_float_d \\
-Wl,--wrap=__nsan_fcmp_fail_double_q \\
-Wl,--wrap=__nsan_fcmp_fail_double_l \\
-Wl,--wrap=__nsan_fcmp_fail_longdouble_q"
EOF
ok "wrote $ROOT/nsan_env.sh"

#==========================================================================
hr "summary"
#==========================================================================
printf '  runtime present : %s\n' "$([ -n "$(find_nsan_rt)" ] && echo yes || echo NO)" | tee -a "$LOG"
printf '  sources         : %s\n' "$SRC" | tee -a "$LOG"
printf '  artifacts       : %s\n' "$ROOT" | tee -a "$LOG"
printf '  log             : %s\n' "$LOG" | tee -a "$LOG"
