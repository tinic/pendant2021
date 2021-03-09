#!/bin/sh
mkdir -p build_debug
cd build_debug
cmake -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=../arm-gcc-toolchain.cmake -DCMAKE_BUILD_TYPE=Debug ..
ninja
cd ..
mkdir -p build_release
cd build_release
cmake -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=../arm-gcc-toolchain.cmake -DCMAKE_BUILD_TYPE=Release ..
ninja
cd ..
