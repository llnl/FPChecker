#!/usr/bin/env bash
# Build the BranchTrace plugin and runtime. Requires LLVM 19 on PATH.
set -euo pipefail
cd "$(dirname "$0")"
: "${CXX:=clang++}"
: "${CC:=clang}"

echo "== llvm-config $(llvm-config --version)"
$CXX -fPIC -shared -o libBranchTrace_mtu.so pass/BranchTrace_mtu.cpp \
    $(llvm-config --cxxflags --ldflags) -Wl,-znodelete
$CC -O2 -c runtime/brtrace_runtime_mtu.c -o brtrace_runtime_mtu.o
echo "done: libBranchTrace_mtu.so  brtrace_runtime_mtu.o"
