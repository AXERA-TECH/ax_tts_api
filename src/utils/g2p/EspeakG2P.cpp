/**************************************************************************************************
 *
 * Copyright (c) 2019-2026 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include "utils/g2p/EspeakG2P.hpp"
#include "utils/logger.h"

namespace utils {

thread_local int32_t EspeakG2P::instance_counter_ = 0;
std::mutex EspeakG2P::global_espeak_mutex_;

std::string EspeakG2P::run(const std::string& input_text, const std::string& language, int& err) {
    std::lock_guard<std::mutex> lock(global_espeak_mutex_);

    voice_properties_.languages = language.c_str();
    err = espeak_SetVoiceByProperties(&voice_properties_);
    if (err != EE_OK) {
        ALOGE("espeak_SetVoiceByProperties failed! language is %s", language.c_str());
        return std::string("");
    }

    int phonememode = ('_' << 8) | 0x02; // 0x02 means IPA, ('_' << 8) means using _ as seperator
    
    std::string phonemes;
    phonemes.reserve(input_text.length() * 2);
    const char* text_ptr = input_text.c_str();
    while (text_ptr != NULL) {
        const char* out_ptr = espeak_TextToPhonemes(
            reinterpret_cast<const void **>(&text_ptr), espeakCHARS_UTF8, phonememode);
        phonemes.append(out_ptr).append("__");
    }
    
    return phonemes;
}

} // namespace utils