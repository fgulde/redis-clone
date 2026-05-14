#!/bin/sh
set -e
cd "$(dirname "$0")"

cmake -B build -S . -G Ninja -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
cmake --build ./build --target redis_tests
cd build && ctest --output-on-failure
