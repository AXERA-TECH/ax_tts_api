#pragma once
#include <memory>
#include <string>
#include <vector>
#include <map>
#include "api/ax_tts_api.h"

class TTSFrontend;

class Kokoro {
public:
    Kokoro();
    ~Kokoro();
    bool init(AX_TTS_TYPE_E tts_type, AX_TTS_INIT_CONFIG* init_config);
    void uninit();
    bool run(const std::string& text, AX_TTS_RUN_CONFIG* config, AX_TTS_AUDIO** audio);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    std::shared_ptr<TTSFrontend> frontend_;
    std::map<std::string, int> vocab_;
    bool load_vocab_(const std::string& path);
};
