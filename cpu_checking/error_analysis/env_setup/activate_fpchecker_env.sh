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

FPC_ENV="${FPC_ENV:-fpchecker_env}"
FPC_SRC="${FPC_SRC:-$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)}"

if [ ! -f "$CONDA_ROOT/etc/profile.d/conda.sh" ]; then
    echo "conda.sh not found under $CONDA_ROOT -- set CONDA_ROOT" >&2
    return 1 2>/dev/null || exit 1
fi

command -v module >/dev/null 2>&1 && module unload llvm_dev/old 2>/dev/null

source "$CONDA_ROOT/etc/profile.d/conda.sh"
conda activate "$FPC_ENV"

export FPC_SRC
export PATH="$FPC_SRC/install/bin:$PATH"
export LD_LIBRARY_PATH="$CONDA_PREFIX/lib:$FPC_SRC/install/lib64:$LD_LIBRARY_PATH"

export PATH=$(echo "$PATH" | awk -v RS=':' 'NF && !seen[$0]++' | paste -sd:)
export LD_LIBRARY_PATH=$(echo "$LD_LIBRARY_PATH" | awk -v RS=':' 'NF && !seen[$0]++' | paste -sd:)

hash -r
echo "fpchecker_env ready."
echo "  FPC_SRC     = $FPC_SRC"
echo "  llvm-config = $(llvm-config --version 2>/dev/null)"
echo "  frontend    = $(command -v clang++-fpchecker || echo '<not built>')"
