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

EFT_ENV="${EFT_ENV:-eftsan_env}"

if [ -z "${EFT_HOME:-}" ]; then
    _here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    for _cand in "$_here/../../../../EFTSanitizer" "$_here/../../../EFTSanitizer"; do
        [ -d "$_cand" ] && EFT_HOME="$(cd "$_cand" && pwd)" && break
    done
    EFT_HOME="${EFT_HOME:-$HOME/EFTSanitizer}"
    unset _here _cand
fi

if [ ! -f "$CONDA_ROOT/etc/profile.d/conda.sh" ]; then
    echo "conda.sh not found under $CONDA_ROOT -- set CONDA_ROOT" >&2
    return 1 2>/dev/null || exit 1
fi

source "$CONDA_ROOT/etc/profile.d/conda.sh"
conda activate "$EFT_ENV"

export CC=clang
export CXX=clang++
export LLVM_HOME="$CONDA_PREFIX"
export PATH="$LLVM_HOME/bin:$PATH"

GCC_VER=$(ls "$CONDA_PREFIX/lib/gcc/x86_64-conda-linux-gnu/" 2>/dev/null | head -1)
if [ -n "$GCC_VER" ]; then
    GXX_INC="$CONDA_PREFIX/lib/gcc/x86_64-conda-linux-gnu/$GCC_VER/include/c++"
    export CPLUS_INCLUDE_PATH="$GXX_INC:$GXX_INC/x86_64-conda-linux-gnu:${CPLUS_INCLUDE_PATH:-}"
else
    echo "WARNING: conda gcc include dir not found -- clang will not find libstdc++ headers" >&2
fi

export CPATH="$CONDA_PREFIX/include:${CPATH:-}"
export LIBRARY_PATH="$CONDA_PREFIX/lib:${LIBRARY_PATH:-}"
export EFT_HOME
export LD_LIBRARY_PATH="$EFT_HOME/runtime/obj:$CONDA_PREFIX/lib:${LD_LIBRARY_PATH:-}"
export LLVM_PASS_LIB=" $EFT_HOME/llvm_pass/build/EFTSan/libEFTSanitizer.so -eftsan "

export PATH=$(echo "$PATH" | awk -v RS=':' 'NF && !seen[$0]++' | paste -sd:)
export LD_LIBRARY_PATH=$(echo "$LD_LIBRARY_PATH" | awk -v RS=':' 'NF && !seen[$0]++' | paste -sd:)

hash -r
echo "eftsan_env ready."
echo "  EFT_HOME     = $EFT_HOME"
echo "  clang        = $(clang --version 2>/dev/null | head -1 | awk '{print $3}')"
echo "  gcc headers  = ${GCC_VER:-<none>}"
echo "  pass         = $([ -f "$EFT_HOME/llvm_pass/build/EFTSan/libEFTSanitizer.so" ] && echo built || echo '<not built>')"
