#!/usr/bin/env bash

# use the cross file based on the architecture, we assume homebrew installs packages in /usr/local on x86_64 and /opt/homebrew on arm64
meson setup --cross-file macos-$(uname -m).ini build

meson compile -C build
