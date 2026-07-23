"""
setuptools setup script for ax-tts.

This script coexists with pyproject.toml: pyproject.toml holds package metadata
and dependencies, while setup.py adds platform detection when building from source.

When a pre-built wheel is available for the user's platform, pip installs it
directly and this script is never executed.  It only runs when pip falls back
to building from source (no matching wheel found).
"""

import os
import sys
from pathlib import Path

from setuptools import setup

HERE = Path(__file__).resolve().parent
PACKAGE_DIR = HERE / "src" / "ax_tts"

# ---- platform / .so detection ------------------------------------------------

so_files = list(PACKAGE_DIR.glob("_ax_tts_core*.so"))

if not so_files:
    machine = os.uname().machine if hasattr(os, "uname") else "unknown"
    is_linux_aarch64 = sys.platform == "linux" and machine == "aarch64"

    if not is_linux_aarch64:
        msg = (
            f"\n{'=' * 72}\n"
            f"  ax-tts 仅支持 aarch64 Linux 平台。\n"
            f"  当前平台: {sys.platform} / {machine}\n"
            f"\n"
            f"  请使用预编译 wheel 安装，或在 aarch64 Linux 设备上从源码构建。\n"
            f"  预编译 wheel 下载地址:\n"
            f"    https://github.com/AXERA-TECH/ax_tts_api/releases\n"
            f"\n"
            f"  从源码构建请参考:\n"
            f"    https://github.com/AXERA-TECH/ax_tts_api#编译\n"
            f"{'=' * 72}\n"
        )
    else:
        msg = (
            f"\n{'=' * 72}\n"
            f"  未找到 _ax_tts_core.so，请先使用 CMake 编译 C++ 扩展。\n"
            f"\n"
            f"  示例 (AX650):\n"
            f"    cd {HERE.parent}\n"
            f"    mkdir -p build && cd build\n"
            f"    cmake ../cpp \\\n"
            f"      -DCHIP_AX650=ON \\\n"
            f"      -DCMAKE_BUILD_TYPE=Release \\\n"
            f"      -DBUILD_PYTHON_BINDINGS=ON\n"
            f"    make -j$(nproc)\n"
            f"    cp build/python/_ax_tts_core*.so python/src/ax_tts/\n"
            f"\n"
            f"  详细编译文档:\n"
            f"    https://github.com/AXERA-TECH/ax_tts_api#编译\n"
            f"{'=' * 72}\n"
        )

    raise RuntimeError(msg)


# ---- build -------------------------------------------------------------------

setup()
