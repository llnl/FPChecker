#!/bin/bash

if [ -z "${CONDA_ROOT:-}" ]; then
    if command -v conda >/dev/null 2>&1; then
        CONDA_ROOT="$(conda info --base)"
    elif [ -n "${CONDA_EXE:-}" ]; then
        CONDA_ROOT="$(dirname "$(dirname "$CONDA_EXE")")"
    elif [ -d /opt/conda ]; then
        CONDA_ROOT=/opt/conda
    else
        CONDA_ROOT="$HOME/miniconda3"
    fi
fi

NSAN_ENV="${NSAN_ENV:-nsan_env}"
_here="$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd)"
NSAN_BF_HOME="${NSAN_BF_HOME:-$(cd "$_here/../branch_flip/experiments/nsan_experiments/nsan" && pwd)}"
unset _here

if [ ! -f "$CONDA_ROOT/etc/profile.d/conda.sh" ]; then
    echo "conda.sh not found under $CONDA_ROOT -- set CONDA_ROOT" >&2
    return 1 2>/dev/null || exit 1
fi

source "$CONDA_ROOT/etc/profile.d/conda.sh"
conda activate "$NSAN_ENV"

export CC=clang
export CXX=clang++
export LLVM_HOME="$CONDA_PREFIX"
export PATH="$LLVM_HOME/bin:$PATH"
export NSAN_BF_HOME
export NSAN_BF_PLUGIN="$NSAN_BF_HOME/plugin/libNsanBFSites.so"
export NSAN_BF_RUNTIME="$NSAN_BF_HOME/runtime/libnsan_bf.a"
export NSAN_WRAP_FLAGS="-Wl,--wrap=__nsan_fcmp_fail_float_d -Wl,--wrap=__nsan_fcmp_fail_double_q -Wl,--wrap=__nsan_fcmp_fail_double_l -Wl,--wrap=__nsan_fcmp_fail_longdouble_q"

# With an unlimited stack the kernel uses a bottom-up mmap layout and ld.so
# lands inside nsan's MAP_FIXED shadow region.
_cur=$(ulimit -s)
if [ "$_cur" = unlimited ] || [ "$_cur" -gt 65536 ] 2>/dev/null; then
    ulimit -s 65536
fi
unset _cur

[ -f "$NSAN_BF_HOME/nsan_setup/nsan_env.sh" ] && source "$NSAN_BF_HOME/nsan_setup/nsan_env.sh"

export PATH=$(echo "$PATH" | awk -v RS=':' 'NF && !seen[$0]++' | paste -sd:)
hash -r
echo "nsan_env ready."
echo "  clang    = $(clang --version 2>/dev/null | head -1 | awk '{print $3}')"
echo "  runtime  = $(find "$(clang -print-resource-dir)/lib" -name 'libclang_rt.nsan*.a' 2>/dev/null | head -1 | xargs -r basename)"
echo "  plugin   = $([ -f "$NSAN_BF_PLUGIN" ] && echo built || echo '<not built>')"
