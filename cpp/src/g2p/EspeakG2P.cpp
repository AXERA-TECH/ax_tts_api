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
#include <unordered_map>


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

static uint32_t utf8_char_to_cp_(const std::string& c) {
    if (c.empty()) return 0;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(c.data());
    if ((p[0] & 0x80) == 0x00) return p[0];
    if ((p[0] & 0xE0) == 0xC0 && c.size() >= 2) return ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
    if ((p[0] & 0xF0) == 0xE0 && c.size() >= 3) return ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
    if ((p[0] & 0xF8) == 0xF0 && c.size() >= 4) return ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
    return 0;
}

static std::string cp_to_utf8_(uint32_t cp) {
    std::string out;
    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return out;
}

static std::string normalize_ja_kana_char_(const std::string& c) {
    uint32_t cp = utf8_char_to_cp_(c);
    // Full-width katakana -> hiragana
    if (cp >= 0x30A1 && cp <= 0x30FA) {
        return cp_to_utf8_(cp - 0x60);
    }
    // Katakana iteration marks -> hiragana iteration marks
    if (cp == 0x30FD) return cp_to_utf8_(0x309D);
    if (cp == 0x30FE) return cp_to_utf8_(0x309E);
    return c;
}

static bool is_ja_punct_or_space_(const std::string& c) {
    static const std::unordered_map<std::string, int> k = {
        {" ",1}, {"\t",1}, {"\n",1}, {"\r",1},
        {"。",1}, {"、",1}, {"，",1}, {"．",1},
        {"！",1}, {"？",1}, {"：",1}, {"；",1}, {"…",1},
        {".",1}, {",",1}, {"!",1}, {"?",1}, {":",1}, {";",1},
        {"「",1}, {"」",1}, {"『",1}, {"』",1}, {"（",1}, {"）",1},
        {"(",1}, {")",1}, {"-",1}, {"〜",1}
    };
    return k.find(c) != k.end();
}

static bool is_small_kana_(const std::string& c) {
    static const std::unordered_map<std::string, int> k = {
        {"ぁ",1}, {"ぃ",1}, {"ぅ",1}, {"ぇ",1}, {"ぉ",1},
        {"ゃ",1}, {"ゅ",1}, {"ょ",1}, {"ゎ",1}
    };
    return k.find(c) != k.end();
}

static const std::unordered_map<std::string, std::string>& ja_kana_map_() {
    static const std::unordered_map<std::string, std::string> k = {
        {"あ","a"},{"い","i"},{"う","u"},{"え","e"},{"お","o"},
        {"か","ka"},{"き","ki"},{"く","ku"},{"け","ke"},{"こ","ko"},
        {"さ","sa"},{"し","ɕi"},{"す","su"},{"せ","se"},{"そ","so"},
        {"た","ta"},{"ち","ʧi"},{"つ","ʦu"},{"て","te"},{"と","to"},
        {"な","na"},{"に","ni"},{"ぬ","nu"},{"ね","ne"},{"の","no"},
        {"は","ha"},{"ひ","hi"},{"ふ","ɸu"},{"へ","he"},{"ほ","ho"},
        {"ま","ma"},{"み","mi"},{"む","mu"},{"め","me"},{"も","mo"},
        {"や","ja"},{"ゆ","ju"},{"よ","jo"},
        {"ら","ɾa"},{"り","ɾi"},{"る","ɾu"},{"れ","ɾe"},{"ろ","ɾo"},
        {"わ","wa"},{"を","o"},
        {"が","ɡa"},{"ぎ","ɡi"},{"ぐ","ɡu"},{"げ","ɡe"},{"ご","ɡo"},
        {"ざ","za"},{"じ","ʥi"},{"ず","zu"},{"ぜ","ze"},{"ぞ","zo"},
        {"だ","da"},{"ぢ","ʥi"},{"づ","zu"},{"で","de"},{"ど","do"},
        {"ば","ba"},{"び","bi"},{"ぶ","bu"},{"べ","be"},{"ぼ","bo"},
        {"ぱ","pa"},{"ぴ","pi"},{"ぷ","pu"},{"ぺ","pe"},{"ぽ","po"},
        {"ゔ","vu"},{"ゐ","wi"},{"ゑ","we"},
        {"ぁ","a"},{"ぃ","i"},{"ぅ","u"},{"ぇ","e"},{"ぉ","o"},
        {"きゃ","kja"},{"きゅ","kju"},{"きょ","kjo"},
        {"しゃ","ɕa"},{"しゅ","ɕu"},{"しょ","ɕo"},{"しぇ","ɕe"},
        {"ちゃ","ʧa"},{"ちゅ","ʧu"},{"ちょ","ʧo"},{"ちぇ","ʧe"},
        {"にゃ","nja"},{"にゅ","nju"},{"にょ","njo"},
        {"ひゃ","hja"},{"ひゅ","hju"},{"ひょ","hjo"},
        {"みゃ","mja"},{"みゅ","mju"},{"みょ","mjo"},
        {"りゃ","ɾja"},{"りゅ","ɾju"},{"りょ","ɾjo"},
        {"ぎゃ","ɡja"},{"ぎゅ","ɡju"},{"ぎょ","ɡjo"},
        {"じゃ","ʥa"},{"じゅ","ʥu"},{"じょ","ʥo"},{"じぇ","ʥe"},
        {"びゃ","bja"},{"びゅ","bju"},{"びょ","bjo"},
        {"ぴゃ","pja"},{"ぴゅ","pju"},{"ぴょ","pjo"},
        {"てぃ","ti"},{"でぃ","di"},{"とぅ","tu"},{"どぅ","du"},
        {"ふぁ","ɸa"},{"ふぃ","ɸi"},{"ふぇ","ɸe"},{"ふぉ","ɸo"},
        {"うぃ","wi"},{"うぇ","we"},{"うぉ","wo"},
        {"つぁ","ʦa"},{"つぃ","ʦi"},{"つぇ","ʦe"},{"つぉ","ʦo"},
        {"ゔぁ","va"},{"ゔぃ","vi"},{"ゔぇ","ve"},{"ゔぉ","vo"}
    };
    return k;
}

static std::string first_symbol_(const std::string& ph) {
    auto chars = utils::split_utf8(ph);
    if (chars.empty()) return "";
    return chars[0];
}

static bool is_vowel_symbol_(const std::string& c) {
    return c == "a" || c == "i" || c == "u" || c == "e" || c == "o";
}

static std::string nasal_for_next_(const std::string& next_ph) {
    std::string f = first_symbol_(next_ph);
    if (f == "b" || f == "p" || f == "m") return "m";
    if (f == "k" || f == "ɡ") return "ŋ";
    return "n";
}

static char tail_vowel_(const std::string& ph) {
    for (int i = (int)ph.size() - 1; i >= 0; --i) {
        char c = ph[i];
        if (c == 'a' || c == 'i' || c == 'u' || c == 'e' || c == 'o') return c;
    }
    return 0;
}

static std::string kana_to_phonemes_ja_light_(const std::string& input, bool& ok) {
    ok = true;
    const auto& m = ja_kana_map_();
    auto raw = utils::split_utf8(input);
    std::vector<std::string> chars;
    chars.reserve(raw.size());
    for (const auto& c : raw) {
        chars.emplace_back(normalize_ja_kana_char_(c));
    }

    std::string out;
    out.reserve(input.size() * 2);
    bool geminate = false;
    char last_vowel = 0;

    auto append_space = [&]() {
        if (!out.empty() && out.back() != ' ') out.push_back(' ');
    };

    for (size_t i = 0; i < chars.size(); ++i) {
        const std::string& c = chars[i];
        if (is_ja_punct_or_space_(c)) {
            append_space();
            geminate = false;
            continue;
        }
        if (c == "っ") {
            geminate = true;
            continue;
        }
        if (c == "ー") {
            if (last_vowel != 0) out.push_back(last_vowel);
            continue;
        }
        if (c == "ん") {
            std::string next_ph;
            for (size_t j = i + 1; j < chars.size(); ++j) {
                if (is_ja_punct_or_space_(chars[j]) || chars[j] == "ー") continue;
                if (chars[j] == "っ") continue;
                std::string key = chars[j];
                if (j + 1 < chars.size() && is_small_kana_(chars[j + 1])) {
                    std::string key2 = chars[j] + chars[j + 1];
                    auto it2 = m.find(key2);
                    if (it2 != m.end()) key = key2;
                }
                auto itn = m.find(key);
                if (itn != m.end()) next_ph = itn->second;
                break;
            }
            out += nasal_for_next_(next_ph);
            geminate = false;
            last_vowel = 0;
            continue;
        }

        std::string key = c;
        if (i + 1 < chars.size() && is_small_kana_(chars[i + 1])) {
            std::string key2 = c + chars[i + 1];
            auto it2 = m.find(key2);
            if (it2 != m.end()) {
                key = key2;
                i++;
            }
        }

        auto it = m.find(key);
        if (it == m.end()) {
            ok = false;
            return std::string();
        }
        const std::string& ph = it->second;

        if (geminate) {
            std::string f = first_symbol_(ph);
            if (!is_vowel_symbol_(f) && f != "n" && f != "m" && f != "ŋ") {
                out += "Q";
            }
            geminate = false;
        }

        out += ph;
        char v = tail_vowel_(ph);
        if (v != 0) last_vowel = v;
    }

    return utils::strip(out);
}

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
        bool converted = false;
        phonemes = kana_to_phonemes_ja_light_(input_text, converted);
        if (!converted || phonemes.empty()) {
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
        // For Japanese, keep raw espeak phonemes as much as possible (kokoro vocab is IPA-like),
        // and only normalize known OOV codepoints that can break long-sentence synthesis.
        phonemes.erase(std::remove(phonemes.begin(), phonemes.end(), '_'), phonemes.end());

        std::string norm;
        auto chars = utils::split_utf8(phonemes);
        norm.reserve(phonemes.size());
        for (const auto& c : chars) {
            // Drop combining diacritics (U+0300..U+036F). They are OOV in kokoro vocab
            // and otherwise get silently skipped later, hurting pronunciation stability.
            if (c.size() == 2) {
                unsigned char b0 = static_cast<unsigned char>(c[0]);
                unsigned char b1 = static_cast<unsigned char>(c[1]);
                if ((b0 == 0xCC && b1 >= 0x80 && b1 <= 0xBF) ||
                    (b0 == 0xCD && b1 >= 0x80 && b1 <= 0xAF)) {
                    continue;
                }
            }

            // Map precomposed nasal vowels that espeak may output in long Japanese to plain vowels.
            // These are not in kokoro vocab and will otherwise be silently skipped.
            if (c == "ũ") { norm += "u"; continue; }
            if (c == "õ") { norm += "o"; continue; }
            if (c == "ã") { norm += "a"; continue; }
            if (c == "ẽ") { norm += "e"; continue; }
            if (c == "ĩ") { norm += "i"; continue; }

            // Map Japanese voiced alveolo-palatal fricative (ʑ) to a close affricate token (ʥ) in vocab.
            if (c == "ʑ") { norm += "ʥ"; continue; }

            // Some espeak voices emit fronted vowels for Japanese; map to basic vowels.
            if (c == "ä") { norm += "a"; continue; }
            if (c == "ö") { norm += "o"; continue; }
            if (c == "ü") { norm += "u"; continue; }

            norm += c;
        }
        phonemes.swap(norm);
    } else {
        _phonemize_postprocess(phonemes);
    }

    return phonemes;
}
