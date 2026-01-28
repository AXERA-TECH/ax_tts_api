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
E2M_Type EspeakG2P::E2M_ = {
    { R"(ʔˌn\u0329)", "tn" }, 
    { R"(ʔn\u0329)", "tn" }, 
    { R"(ʔn)", "tn" }, 
    { R"(ʔ)", "t" }, 
    { R"(aɪ)", "I" }, 
    { R"(aʊ)", "W" }, 
    { R"(dʒ)", "ʤ" }, 
    { R"(eɪ)", "A" }, 
    { R"(e)", "A" }, 
    { R"(tʃ)", "ʧ" }, 
    { R"(ɔɪ)", "Y" }, 
    { R"(əl)", "ᵊl" }, 
    { R"(ʲo)", "jo" }, 
    { R"(ʲə)", "jə" }, 
    { R"(ʲ)", "" }, 
    { R"(ɚ)", "əɹ" }, 
    { R"(r)", "ɹ" }, 
    { R"(x)", "k" }, 
    { R"(ç)", "k" }, 
    { R"(ɐ)", "ə" }, 
    { R"(ɬ)", "l" }, 
    { R"(\u0303)", "" }, 
    { R"(oʊ)", "O" }, 
    { R"(ɜːɹ)", "ɜɹ" }, 
    { R"(ɜː)", "ɜɹ" }, 
    { R"(ɪə)", "iə" }, 
    { R"(ː)", "" } 
};

std::string EspeakG2P::run(const std::string& input_text, const std::string& language, int& err) {
    std::lock_guard<std::mutex> lock(global_espeak_mutex_);

    voice_properties_.languages = language.c_str();
    err = espeak_SetVoiceByProperties(&voice_properties_);
    if (err != EE_OK) {
        ALOGE("espeak_SetVoiceByProperties failed! language is %s", language.c_str());
        return std::string("");
    }

    // 0x02 means IPA, ('_' << 8) means using _ as seperator
    int phonememode = 0x02 | ('_' << 8);
    
    std::string phonemes;
    phonemes.reserve(input_text.length() * 2);

    // 分割标点
    auto line_marks = _phonemize_preprocess(input_text);

    for (size_t i = 0; i < line_marks.size(); i++) {
        const char* text_ptr = line_marks[i].first.c_str();

        while (text_ptr != NULL) {
            const char* out_ptr = espeak_TextToPhonemes(
                reinterpret_cast<const void **>(&text_ptr), espeakCHARS_AUTO, phonememode);
            phonemes.append(out_ptr);
        }

        // 添加回标点
        phonemes.append(line_marks[i].second);

        // 断句之间添加空格
        if (i < line_marks.size() - 1) {
            phonemes.append(std::string(" "));
        }
    }

    // 后处理, 替换部分音素使其更自然
    _phonemize_postprocess(phonemes);
    
    return phonemes;
}

} // namespace utils