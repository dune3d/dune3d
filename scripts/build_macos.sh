#!/usr/bin/env bash

llvm_prefix="$(brew --prefix llvm)"
export CC="$llvm_prefix/bin/clang"
export CXX="$llvm_prefix/bin/clang++"
export PATH="$llvm_prefix/bin:$PATH"
export LDFLAGS="-L$llvm_prefix/lib/c++ -Wl,-rpath,$llvm_prefix/lib/c++"

meson setup build
meson compile -C build
