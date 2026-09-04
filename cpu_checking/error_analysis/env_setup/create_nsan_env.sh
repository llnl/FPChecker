#!/bin/bash
# create_nsan_env.sh -- clone fpchecker_env into nsan_env.
#
#   ./create_nsan_env.sh              # clone fpchecker_env -> nsan_env
#   ./create_nsan_env.sh --recreate   # delete and rebuild
#
# A clone, so nsan uses the byte-identical clang 19.1.7 that brtrace and
# FPChecker use. nsan_setup.sh then installs libclang_rt.nsan into this
# env's resource dir, leaving fpchecker_env untouched.

set -uo pipefail

SRC_ENV="${SRC_ENV:-fpchecker_env}"
DST_ENV="nsan_env"
RECREATE=0
if [ -z "${CONDA_ROOT:-}" ]; then
  if command -v conda >/dev/null 2>&1; then CONDA_ROOT="$(conda info --base)"
  elif [ -n "${CONDA_EXE:-}" ]; then CONDA_ROOT="$(dirname "$(dirname "$CONDA_EXE")")"
  elif [ -d /opt/conda ]; then CONDA_ROOT=/opt/conda
  else CONDA_ROOT="$HOME/miniconda3"; fi
fi
CONDA_SH="${CONDA_SH:-$CONDA_ROOT/etc/profile.d/conda.sh}"

while [ $# -gt 0 ]; do
  case "$1" in
    --recreate) RECREATE=1 ;;
    --name)     shift; DST_ENV="$1" ;;
    --from)     shift; SRC_ENV="$1" ;;
    -h|--help)  sed -n '2,9p' "$0"; exit 0 ;;
    *) echo "unknown argument: $1"; exit 2 ;;
  esac
  shift
done

ok()   { printf '  [ ok ] %s\n' "$1"; }
warn() { printf '  [warn] %s\n' "$1"; }
die()  { printf '  [FAIL] %s\n' "$1"; exit 1; }
hr()   { printf '\n=== %s ===\n' "$1"; }

hr "conda"
[ -f "$CONDA_SH" ] || die "conda.sh not at $CONDA_SH (set CONDA_ROOT)"
# shellcheck disable=SC1090
source "$CONDA_SH"
ok "sourced $CONDA_SH"

conda env list | grep -qE "^${SRC_ENV}\s" \
  || die "source env '$SRC_ENV' does not exist"
ok "source env: $SRC_ENV"

hr "target env"
if conda env list | grep -qE "^${DST_ENV}\s"; then
  if [ "$RECREATE" = 1 ]; then
    warn "removing existing $DST_ENV"
    conda env remove -n "$DST_ENV" -y >/dev/null 2>&1 || die "remove failed"
  else
    ok "$DST_ENV already exists (use --recreate to rebuild)"
    SKIP_CREATE=1
  fi
fi

if [ "${SKIP_CREATE:-0}" != 1 ]; then
  echo "  cloning $SRC_ENV -> $DST_ENV (hardlinked; a few minutes)"
  conda create --clone "$SRC_ENV" -n "$DST_ENV" -y \
    || die "clone failed"
  ok "cloned"
fi

hr "build dependencies"
conda activate "$DST_ENV" || die "cannot activate $DST_ENV"

NEED=""
for pkg in cmake ninja; do
  command -v "$pkg" >/dev/null || NEED="$NEED $pkg"
done
if [ -n "$NEED" ]; then
  echo "  installing:$NEED"
  conda install -n "$DST_ENV" -c conda-forge -y $NEED >/dev/null 2>&1 \
    || warn "install of$NEED failed; compiler-rt build may not configure"
fi
for pkg in cmake ninja; do
  if command -v "$pkg" >/dev/null; then ok "$pkg $($pkg --version 2>/dev/null | head -1)"
  else warn "$pkg still missing"; fi
done

hr "verify the toolchain matches the source env"
CLANG_VER=$(clang --version 2>/dev/null | sed -n '1s/.*version \([0-9.]*\).*/\1/p')
PREFIX=$(cd "$(dirname "$(readlink -f "$(command -v clang)")")/.." && pwd)
RESDIR=$(clang -print-resource-dir 2>/dev/null)
echo "  clang        $CLANG_VER"
echo "  prefix       $PREFIX"
echo "  resource dir $RESDIR"

case "$CLANG_VER" in
  19.*) ok "LLVM 19 -- nsan available as -fsanitize=numerical" ;;
  *)    die "expected clang 19.x, got '$CLANG_VER'" ;;
esac

MY_HASH=$(sha256sum "$(readlink -f "$(command -v clang)")" | cut -c1-16)
conda activate "$SRC_ENV" 2>/dev/null
SRC_HASH=$(sha256sum "$(readlink -f "$(command -v clang)")" 2>/dev/null | cut -c1-16)
conda activate "$DST_ENV"
if [ "$MY_HASH" = "$SRC_HASH" ]; then
  ok "clang binary identical to $SRC_ENV ($MY_HASH)"
else
  warn "clang differs from $SRC_ENV ($MY_HASH vs $SRC_HASH); re-verify site ids against brtrace"
fi

if command -v llvm-config >/dev/null; then
  ok "llvm-config present, cmakedir: $(llvm-config --cmakedir 2>/dev/null)"
else
  warn "llvm-config absent -- standalone compiler-rt build will not configure"
  echo "         fix: conda install -n $DST_ENV -c conda-forge llvmdev=$CLANG_VER"
fi

if find "$RESDIR/lib" -name 'libclang_rt.nsan*' 2>/dev/null | grep -q .; then
  ok "nsan runtime already present"
else
  warn "nsan runtime absent -- nsan_setup.sh will build it into THIS env only"
fi

hr "next"
echo "  source env_setup/activate_nsan_env.sh"
echo "  cd branch_flip/experiments/nsan_experiments/nsan && ./nsan_setup.sh -j 32 && ./build_instrumentation.sh"
