#!/bin/bash

# ============================================================
# 下载 BSP SDK 和第三方编译依赖
# ============================================================

if [ ! -d ax650n_bsp_sdk ]; then
  echo "clone ax650 bsp to ax650n_bsp_sdk, please wait..."
  git clone https://github.com/AXERA-TECH/ax650n_bsp_sdk.git --depth=1
fi

if [ ! -d ax620e_bsp_sdk ]; then
  echo "clone ax620e bsp to ax620e_bsp_sdk, please wait..."
  git clone https://github.com/AXERA-TECH/ax620e_bsp_sdk.git --depth=1
fi

if [ ! -d axcl_bsp_sdk ]; then
  echo "clone axcl bsp to axcl_bsp_sdk, please wait..."
  git clone https://github.com/Abandon-ht/axcl_bsp_sdk.git --depth=1
fi

# ============================================================
# 第三方预编译库 (espeak-ng, onnxruntime)
#
# aarch64 库已随仓库提供在 cpp/third_party/ 下。
# arm-uclibc / arm-gnueabihf 库需从旧 commit 中提取，
# 或手动编译后放入对应目录:
#   cpp/third_party/espeak-ng/lib/uclibc/
#   cpp/third_party/espeak-ng/lib/gnueabihf/
#   cpp/third_party/onnxruntime-.../lib/arm_uclibc/
#   cpp/third_party/onnxruntime-.../lib/arm_gnueabihf/
#
# 从 git 历史恢复（如果可用）:
COMMIT_WITH_ARM_LIBS="4459846"  # refactor 前的最后一个 commit
THIRDPARTY_DIR="cpp/third_party"

restore_arch_libs() {
  local arch="$1"  # uclibc or gnueabihf
  local espeak_dir="$THIRDPARTY_DIR/espeak-ng/lib/$arch"
  local ort_dir="$THIRDPARTY_DIR/onnxruntime-linux-aarch64-static_lib-1.16.0/lib/arm_$arch"

  if [ ! -d "$espeak_dir" ]; then
    echo "Restoring espeak-ng $arch libs from git history..."
    git checkout "$COMMIT_WITH_ARM_LIBS" -- "$espeak_dir" 2>/dev/null || \
      echo "WARNING: cannot restore $espeak_dir. Build espeak-ng for $arch manually."
  fi

  if [ "$arch" = "gnueabihf" ] && [ ! -d "$ort_dir" ]; then
    echo "Restoring onnxruntime $arch libs from git history..."
    git checkout "$COMMIT_WITH_ARM_LIBS" -- "$ort_dir" 2>/dev/null || \
      echo "WARNING: cannot restore $ort_dir. Build onnxruntime for $arch manually."
  fi
}

if [ "$1" = "--all-arch" ]; then
  echo "Restoring arm third-party libs..."
  restore_arch_libs "uclibc"
  restore_arch_libs "gnueabihf"
fi

echo ""
echo "BSP download complete."
echo "For AX620Q (arm) builds, run: bash download_bsp.sh --all-arch"
