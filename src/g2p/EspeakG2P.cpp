/**************************************************************************************************
 *
 * Copyright (c) 2019-2026 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include "g2p/EspeakG2P.hpp"
#include "utils/logger.h"
#include "utils/string_utils.hpp"
#include <algorithm>
#include <cstdlib>


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

    // Reset voice properties every run to avoid carrying over settings between languages.
    memset(&voice_properties_, 0, sizeof(voice_properties_));

    bool is_ja = (language == "ja" || language.rfind("ja-", 0) == 0 || language.rfind("ja_", 0) == 0);
    if (is_ja) {
        voice_properties_.languages = "ja";
        voice_properties_.gender = 2;
        voice_properties_.age = 0;
        voice_properties_.variant = 0;
        voice_properties_.name = NULL;
    } else {
        voice_properties_.languages = language.c_str();
    }
    err = espeak_SetVoiceByProperties(&voice_properties_);
    if (err != EE_OK) {
        ALOGE("espeak_SetVoiceByProperties failed! language is %s", language.c_str());
        return std::string("");
    }

    // 0x02 means IPA, ('_' << 8) means using _ as seperator
    int phonememode = 0x02 | ('_' << 8);
    
    std::string phonemes;
    phonemes.reserve(input_text.length() * 2);

    if (is_ja) {
        const std::string& seg = input_text;
        const char* text_ptr = seg.c_str();

        const char* text_end = seg.c_str() + seg.length();
        int iteration = 0;
        while (text_ptr && *text_ptr && text_ptr < text_end) {
            const char* before_ptr = text_ptr;
            const char* out_ptr = espeak_TextToPhonemes(
                reinterpret_cast<const void **>(&text_ptr), espeakCHARS_UTF8, phonememode);
            if (out_ptr) {
                if (iteration > 0 && !phonemes.empty()) {
                    phonemes.append("   ");
                }
                phonemes.append(out_ptr);
            }
            if (text_ptr == before_ptr) {
                ALOGW("espeak_TextToPhonemes did not advance pointer, breaking to avoid infinite loop");
                if (*text_ptr) text_ptr++;
            }
            if (++iteration > 100) {
                ALOGE("Too many iterations in phonemize, breaking");
                break;
            }
        }
    } else {
        // 分割标点
        auto line_marks = _phonemize_preprocess(input_text);

        for (size_t i = 0; i < line_marks.size(); i++) {
            const std::string& seg = line_marks[i].first;
            const char* text_ptr = seg.c_str();
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
    }

    // 后处理, 替换部分音素使其更自然
    if (is_ja) {
        // For Japanese, keep raw espeak phonemes but drop "_" separators.
        phonemes.erase(std::remove(phonemes.begin(), phonemes.end(), '_'), phonemes.end());
        // Normalize ja phonemes to vocab-friendly symbols.
        std::string norm;
        auto chars = utils::split_utf8(phonemes);
        norm.reserve(phonemes.size());
        for (const auto& c : chars) {
            // Drop combining diacritics (U+0300..U+036F)
            if (c.size() == 2) {
                unsigned char b0 = static_cast<unsigned char>(c[0]);
                unsigned char b1 = static_cast<unsigned char>(c[1]);
                if ((b0 == 0xCC && b1 >= 0x80 && b1 <= 0xBF) ||
                    (b0 == 0xCD && b1 >= 0x80 && b1 <= 0xAF)) {
                    continue;
                }
            }
            if (c == "ˈ" || c == "ˌ" || c == "ᵝ" || c == "ʲ") {
                continue;
            }
            if (c == "ɯ") norm += "u";
            else if (c == "ä") norm += "a";
            else if (c == "ö") norm += "o";
            else if (c == "ü") norm += "u";
            else if (c == "Ä") norm += "A";
            else if (c == "Ö") norm += "O";
            else if (c == "Ü") norm += "U";
            else norm += c;
        }
        phonemes.swap(norm);
    } else {
        _phonemize_postprocess(phonemes);
    }

    return phonemes;
}
