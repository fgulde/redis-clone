set shell := ["bash", "-cu"]

default: help

help:
    @just --list

build:
    cmake -B build -S . -G Ninja -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    cmake --build ./build

run *args: build
    ./build/redis {{args}}

test:
    cmake -B build -S . -G Ninja -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    cmake --build ./build --target redis_tests
    cd build && ctest --output-on-failure

clean:
    rm -rf build