#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
# Point SOKOL_SHDC at the official floooh/sokol-tools-bin executable.
shader_compiler=${SOKOL_SHDC:-sokol-shdc}
"$shader_compiler" -i src/shaders/shaders.glsl -o src/shaders/shaders.glsl.h -l metal_macos
"$shader_compiler" -i modules/xvg/xvg_shaders.glsl -o src/shaders/xvg_shaders.glsl.h -l metal_macos
cmake -S . -B build/custom -DCMAKE_BUILD_TYPE=Release
cmake --build build/custom --target ScreamCustom_auv2 -j 6
codesign --force --sign - build/custom/Release/ScreamCustom.component
codesign --verify --strict build/custom/Release/ScreamCustom.component
