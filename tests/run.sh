#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/tests
clang -std=c99 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc tests/test_smoother.c src/spectral_smoother.c src/libs/pffft.c -o build/tests/test_smoother
build/tests/test_smoother
