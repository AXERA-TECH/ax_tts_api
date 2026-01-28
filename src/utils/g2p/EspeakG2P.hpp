/**************************************************************************************************
 *
 * Copyright (c) 2019-2026 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#pragma once

#include <atomic>
#include <mutex>
#include <string.h>
#include "espeak-ng/speak_lib.h"
#include "utils/g2p/Punctuator.hpp"
#include "utils/string_utils.hpp"

namespace utils {

typedef std::map<std::string, std::string>    E2M_Type;

// Following https://github.com/hexgrad/misaki/blob/main/misaki/espeak.py
class EspeakG2P {
private:
    // 每个实例有自己的espeak上下文
    static thread_local int32_t instance_counter_;
    // 线程安全锁
    static std::mutex global_espeak_mutex_;
    // misaki中默认要替换的因素, E2M means eSpeak to Misaki
    static E2M_Type E2M_;

    // 语音属性（每个实例独立）
    espeak_VOICE voice_properties_;
    // 音素分隔符
    std::string tie_;
    // 标点分割器
    Punctuator punc_;
    
public:
    EspeakG2P(const char* espeak_data_path = "./espeak-ng-data"):
        tie_("^")
    {
        if (instance_counter_ == 0) {
            espeak_Initialize(AUDIO_OUTPUT_RETRIEVAL, 0, espeak_data_path, 0);
        }

        memset(&voice_properties_, 0, sizeof(voice_properties_));
        ++instance_counter_;
    }

    ~EspeakG2P() {
        --instance_counter_;
        if (instance_counter_ == 0) {
            espeak_Terminate();
        }
    }

    std::string run(const std::string& input_text, const std::string& language, int& err);

    inline void set_tie(const std::string& tie) { tie_ = tie; }

protected:
    virtual std::vector<LineMarkPair> _phonemize_preprocess(const std::string& text) {
        auto line_marks = punc_.run(text);

        // Remove empty lines
        for (auto it = line_marks.begin(); it != line_marks.end();) {
            if (it->first.empty())
                it = line_marks.erase(it);
            else {
                it->first = strip(it->first);
                it++;
            }  
        }

        return line_marks;
    }

    virtual std::string _phonemize_postprocess(std::string& phonemes) {
        // replace phonemes in E2M
        for (auto& [key, value]: E2M_) {
            phonemes = std::regex_replace(phonemes, std::regex(key), value);
        }
        phonemes = std::regex_replace(phonemes, std::regex(R"(_)"), "");
        return phonemes;
    }
};

} // namespace utils