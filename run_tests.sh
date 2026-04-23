#!/bin/sh
set -e
cd "$(dirname "$0")"

: "${VCPKG_ROOT:=/home/fgulde/vcpkg}"

cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
cmake --build ./build --target redis_tests
cd build && ctest --output-on-failure
