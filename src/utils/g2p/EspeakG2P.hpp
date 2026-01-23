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

namespace utils {

class EspeakG2P {
private:
    // 每个实例有自己的espeak上下文
    static thread_local int32_t instance_counter_;
    
    // 语音属性（每个实例独立）
    espeak_VOICE voice_properties_;
    
    // 线程安全锁
    static std::mutex global_espeak_mutex_;

public:
    EspeakG2P(const char* espeak_data_path = "./espeak-ng-data"){
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
};

} // namespace utils