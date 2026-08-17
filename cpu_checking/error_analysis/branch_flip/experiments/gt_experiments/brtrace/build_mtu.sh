#!/usr/bin/env bash
# Build the MULTI-TU BranchTrace plugin + runtime.
# Produces libBranchTrace_mtu.so and brtrace_runtime_mtu.o, leaving the
# single-TU libBranchTrace.so untouched.
set -euo pipefail
cd "$(dirname "$0")"
: "${CXX:=clang++}"
: "${CC:=clang}"

echo "== llvm-config $(llvm-config --version)"
echo "== plugin (libBranchTrace_mtu.so)"
$CXX -fPIC -shared -o libBranchTrace_mtu.so pass/BranchTrace_mtu.cpp \
    $(llvm-config --cxxflags --ldflags) -Wl,-znodelete
echo "== runtime (brtrace_runtime_mtu.o)"
$CC -O2 -c runtime/brtrace_runtime_mtu.c -o brtrace_runtime_mtu.o
echo "done: libBranchTrace_mtu.so  brtrace_runtime_mtu.o"
