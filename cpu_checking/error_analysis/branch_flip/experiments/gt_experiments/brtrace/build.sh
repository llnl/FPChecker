#!/usr/bin/env bash
# Build the BranchTrace pass plugin and the runtime object.
# Requires LLVM 19.x on PATH (e.g. your fpchecker_env: llvm-config, clang).
set -euo pipefail
cd "$(dirname "$0")"

: "${CXX:=clang++}"
: "${CC:=clang}"

echo "== llvm-config: $(llvm-config --version)"

echo "== building pass plugin (libBranchTrace.so)"
$CXX -fPIC -shared -o libBranchTrace.so pass/BranchTrace.cpp \
    $(llvm-config --cxxflags --ldflags) \
    -Wl,-znodelete

echo "== building runtime (brtrace_runtime.o)"
$CC -O2 -c runtime/brtrace_runtime.c -o brtrace_runtime.o

echo "done:"
echo "  ./libBranchTrace.so"
echo "  ./brtrace_runtime.o"
