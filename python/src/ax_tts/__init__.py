"""
Axera TTS Python API.

Provides AX_TTS class wrapping the C++ TTS engine via pybind11.
"""

from typing import Tuple
import numpy as np

from . import _ax_tts_core


class AX_TTS:
    """Axera TTS engine wrapper.

    Usage::

        tts = AX_TTS(model_path="models-ax650", espeak_data_path="espeak-ng-data")
        sr, audio = tts.synthesize("Hello world", language="en", voice="af_heart")
        tts.close()
    """

    def __init__(
        self,
        model_path: str = "models-ax630c",
        espeak_data_path: str = "espeak-ng-data",
        jieba_dict_path: str = "dict",
        max_seq_len: int = 96,
        tts_type: str = "KOKORO",
    ):
        tts_type_enum = getattr(_ax_tts_core.TTSType, tts_type.upper(), _ax_tts_core.TTSType.KOKORO)

        config = _ax_tts_core.InitConfig()
        config.max_seq_len = max_seq_len
        config.model_path = model_path
        config.espeak_data_path = espeak_data_path
        config.jieba_dict_path = jieba_dict_path

        self._handle = _ax_tts_core.init(tts_type_enum, config)
        if not self._handle:
            raise RuntimeError("AX_TTS_Init failed")

    def synthesize(
        self,
        text: str,
        language: str = "en",
        voice: str = "af_heart",
        speed: float = 1.0,
        fade_out: float = 0.3,
        sample_rate: int = 24000,
    ) -> Tuple[int, np.ndarray]:
        """Synthesize speech from text.

        Returns:
            Tuple of (sample_rate, numpy float32 array of audio samples).
        """
        config = _ax_tts_core.RunConfig()
        config.speed = speed
        config.fade_out = fade_out
        config.sample_rate = sample_rate
        config.voice = voice
        config.language = language

        sr, audio = _ax_tts_core.run(self._handle, text, config)
        return sr, audio

    def close(self):
        """Release TTS resources."""
        if self._handle:
            _ax_tts_core.uninit(self._handle)
            self._handle = None

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()
