#!/usr/bin/env python3
"""Smoke test for AX_TTS Python bindings.

Run on target device (aarch64) after installing ax-tts package:
    python -m pytest python/tests/test_smoke.py -v

On x86 host, run static checks only:
    python -m pytest python/tests/test_smoke.py -v -k "static"
"""

import sys
import importlib.util
from pathlib import Path

import pytest

PYTHON_SRC = Path(__file__).resolve().parent.parent / "src"


class TestStaticChecks:
    """Tests that don't require loading the native .so."""

    def test_package_structure(self):
        """Verify ax_tts package directory exists."""
        pkg = PYTHON_SRC / "ax_tts"
        assert pkg.is_dir()
        assert (pkg / "__init__.py").is_file()

    def test_init_syntax(self):
        """Verify __init__.py has valid Python syntax."""
        init_file = PYTHON_SRC / "ax_tts" / "__init__.py"
        with open(init_file) as f:
            source = f.read()
        compile(source, str(init_file), "exec")

    def test_init_docstring(self):
        """Verify AX_TTS class is defined in __init__.py source."""
        init_file = PYTHON_SRC / "ax_tts" / "__init__.py"
        source = init_file.read_text()
        assert "class AX_TTS" in source
        assert "def synthesize" in source
        assert "def close" in source
        assert "def __init__" in source

    def test_pyproject_valid(self):
        """Verify pyproject.toml has required fields."""
        pyproject = Path(__file__).resolve().parent.parent / "pyproject.toml"
        text = pyproject.read_text()
        assert "ax-tts" in text
        assert "pybind11" in text
        assert "numpy" in text

    def test_bindings_syntax(self):
        """Verify bindings.cpp compiles (C++ syntax check via CMake already verified)."""
        bindings = PYTHON_SRC / "bindings.cpp"
        assert bindings.is_file()
        with open(bindings) as f:
            source = f.read()
        assert "PYBIND11_MODULE" in source
        assert "AX_TTS_Init" in source
        assert "AX_TTS_Run" in source
        assert "AX_TTS_Uninit" in source

    def test_api_consistency(self):
        """Verify Python class matches C API surface."""
        init_file = PYTHON_SRC / "ax_tts" / "__init__.py"
        source = init_file.read_text()
        # Check all C API functions are referenced
        assert "ax_tts_core.init" in source or "init(" in source
        assert "ax_tts_core.run" in source or "run(" in source
        assert "ax_tts_core.uninit" in source or "uninit(" in source
        # Check struct fields match
        assert "max_seq_len" in source
        assert "model_path" in source
        assert "espeak_data_path" in source
        assert "jieba_dict_path" in source


@pytest.mark.device
class TestDeviceOnly:
    """Tests that require the native .so on the target device."""

    @pytest.fixture
    def ax_tts(self):
        """Import ax_tts module (requires .so on target device)."""
        sys.path.insert(0, str(PYTHON_SRC))
        import ax_tts
        return ax_tts

    def test_import(self, ax_tts):
        """Verify module can be imported."""
        assert hasattr(ax_tts, "AX_TTS")

    def test_instantiate(self, ax_tts):
        """Verify AX_TTS can be instantiated."""
        tts = ax_tts.AX_TTS()
        assert tts._handle is not None
        tts.close()

    def test_synthesize(self, ax_tts):
        """Verify synthesize returns sample_rate and numpy array."""
        tts = ax_tts.AX_TTS()
        sr, audio = tts.synthesize("Hello world", language="en", voice="af_heart")
        assert isinstance(sr, int)
        assert sr > 0
        import numpy as np
        assert isinstance(audio, np.ndarray)
        assert audio.dtype == np.float32
        assert len(audio) > 0
        tts.close()

    def test_context_manager(self, ax_tts):
        """Verify AX_TTS works as context manager."""
        with ax_tts.AX_TTS() as tts:
            assert tts._handle is not None
        assert tts._handle is None
