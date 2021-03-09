#!/bin/sh
set -e
gcc --version | awk '/gcc/ && ($3+0)<10.2{print "GCC must be version 10.2 or higher."; exit 1}'
build_type="Ninja"
rm -rf pendant2021_*
mkdir -p pendant2021_bootloader_dgb
cd pendant2021_bootloader_dgb
cmake -G "$build_type" -DCMAKE_TOOLCHAIN_FILE=../arm-gcc-toolchain.cmake -DBOOTLOADER=1 -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
cd ..
mkdir -p pendant2021_bootloaded_dgb
cd pendant2021_bootloaded_dgb
cmake -G "$build_type" -DCMAKE_TOOLCHAIN_FILE=../arm-gcc-toolchain.cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
cd ..
mkdir -p pendant2021_bootloader_rel
cd pendant2021_bootloader_rel
cmake -G "$build_type" -DCMAKE_TOOLCHAIN_FILE=../arm-gcc-toolchain.cmake -DBOOTLOADER=1 -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
cd ..
mkdir -p pendant2021_bootloaded_rel
cd pendant2021_bootloaded_rel
cmake -G "$build_type" -DCMAKE_TOOLCHAIN_FILE=../arm-gcc-toolchain.cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
cd ..
