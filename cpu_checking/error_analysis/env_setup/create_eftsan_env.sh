#!/bin/bash
set -e

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

if [ ! -f "$CONDA_ROOT/etc/profile.d/conda.sh" ]; then
    echo "conda.sh not found under $CONDA_ROOT -- set CONDA_ROOT to your conda install" >&2
    exit 1
fi

source "$CONDA_ROOT/etc/profile.d/conda.sh"

if conda env list | grep -qE "^${EFT_ENV}\s"; then
    echo "$EFT_ENV already exists"
else
conda create -y -n "$EFT_ENV" -c conda-forge \
    'clangdev=10.0.0' 'llvmdev=10.0.0' 'llvm=10.0.0' \
    gmp 'mpfr=4.1.0' cmake make 'gxx_linux-64=15' 'gcc_linux-64=15' python=3.9
fi

conda activate "$EFT_ENV"
clang --version
ls "$CONDA_PREFIX/lib/gcc/x86_64-conda-linux-gnu/"
