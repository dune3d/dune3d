#!/usr/bin/env bash

llvm_prefix="$(brew --prefix llvm)"
export CC="$llvm_prefix/bin/clang"
export CXX="$llvm_prefix/bin/clang++"

meson setup build
meson compile -C build
