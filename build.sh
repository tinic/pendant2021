#!/bin/sh
set -e
build_type="Ninja"
rm -rf pendant2021_*
mkdir -p pendant2021_mainmcu_bootloader_dgb
cd pendant2021_mainmcu_bootloader_dgb
cmake -G "$build_type" -DCMAKE_TOOLCHAIN_FILE=../arm-gcc-toolchain.cmake -DBOOTLOADER=1 -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
cd ..
mkdir -p pendant2021_mainmcu_bootloaded_dgb
cd pendant2021_mainmcu_bootloaded_dgb
cmake -G "$build_type" -DCMAKE_TOOLCHAIN_FILE=../arm-gcc-toolchain.cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
cd ..
mkdir -p pendant2021_mainmcu_bootloader_rel
cd pendant2021_mainmcu_bootloader_rel
cmake -G "$build_type" -DCMAKE_TOOLCHAIN_FILE=../arm-gcc-toolchain.cmake -DBOOTLOADER=1 -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
cd ..
mkdir -p pendant2021_mainmcu_bootloaded_rel
cd pendant2021_mainmcu_bootloaded_rel
cmake -G "$build_type" -DCMAKE_TOOLCHAIN_FILE=../arm-gcc-toolchain.cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
cd ..
