#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/tests
clang -g tests/test_au.c -framework AudioToolbox -framework CoreFoundation -o build/tests/test_au
build/tests/test_au build/custom/Release/ScreamCustom.component/Contents/MacOS/ScreamCustom
