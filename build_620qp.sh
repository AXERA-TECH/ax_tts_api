#!/bin/bash
set -e

BSP_MSP_DIR_DEFAULT="/home/axera/msp_dir/ax620e/msp/out/arm_glibc"
BSP_MSP_DIR="${BSP_MSP_DIR:-$BSP_MSP_DIR_DEFAULT}"

mkdir -p build_620qp && cd build_620qp
if [ -f CMakeCache.txt ]; then
  cmake ../cpp  \
    -DCHIP_AX620Q=ON  \
    -DBSP_MSP_DIR="${BSP_MSP_DIR}" \
    -DCMAKE_INSTALL_PREFIX=../install/ax620qp \
    -DCMAKE_BUILD_TYPE=Release  \
    "$@"
else
  cmake ../cpp  \
    -DCHIP_AX620Q=ON  \
    -DCMAKE_TOOLCHAIN_FILE=../toolchains/arm-linux-gnueabihf.toolchain.cmake \
    -DBSP_MSP_DIR="${BSP_MSP_DIR}" \
    -DCMAKE_INSTALL_PREFIX=../install/ax620qp \
    -DCMAKE_BUILD_TYPE=Release  \
    "$@"
fi
make -j4
make install
