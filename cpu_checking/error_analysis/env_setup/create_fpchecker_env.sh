#!/bin/bash
set -e

if [ -z "$CONDA_ROOT" ]; then
    if command -v conda >/dev/null 2>&1; then
        CONDA_ROOT="$(conda info --base)"
    elif [ -n "$CONDA_EXE" ]; then
        CONDA_ROOT="$(dirname "$(dirname "$CONDA_EXE")")"
    else
        CONDA_ROOT="$HOME/miniconda3"
    fi
fi

FPC_ENV="${FPC_ENV:-fpchecker_env}"

if [ ! -f "$CONDA_ROOT/etc/profile.d/conda.sh" ]; then
    echo "conda.sh not found under $CONDA_ROOT -- set CONDA_ROOT to your conda install" >&2
    exit 1
fi

source "$CONDA_ROOT/etc/profile.d/conda.sh"

conda create -y -n "$FPC_ENV" -c conda-forge \
    'clangxx=19.1.7' 'llvmdev=19.1.7' \
    'python=3.12.9' openmpi=5.0.7 cmake make git matplotlib

conda activate "$FPC_ENV"
clang++ --version
llvm-config --version
python -c "import matplotlib; print(matplotlib.__version__, matplotlib.__file__)"
