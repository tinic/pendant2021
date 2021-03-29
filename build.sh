#!/bin/sh
export PATH="/opt/gcc-arm-none-eabi-10-2020-q4-major/bin:${PATH}"
set -e
build_type="Ninja"
rm -rf pendant2021_*
mkdir -p pendant2021_bootloader_dgb
cd pendant2021_bootloader_dgb
cmake -G "$build_type" -DCMAKE_TOOLCHAIN_FILE=../arm-gcc-toolchain.cmake -DBOOTLOADER=1 -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
cd ..
mkdir -p pendant2021_bootloaded_dgb
cd pendant2021_bootloaded_dgb
cmake -G "$build_type" -DCMAKE_TOOLCHAIN_FILE=../arm-gcc-toolchain.cmake -DBOOTLOADED=1 -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
cd ..
mkdir -p pendant2021_bootloader_rel
cd pendant2021_bootloader_rel
cmake -G "$build_type" -DCMAKE_TOOLCHAIN_FILE=../arm-gcc-toolchain.cmake -DBOOTLOADER=1 -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
cd ..
mkdir -p pendant2021_bootloaded_rel
cd pendant2021_bootloaded_rel
cmake -G "$build_type" -DCMAKE_TOOLCHAIN_FILE=../arm-gcc-toolchain.cmake -DBOOTLOADED=1 -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
cd ..
