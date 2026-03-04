/**************************************************************************************************
 *
 * Copyright (c) 2019-2026 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include "frontend/kokoro_frontend.hpp"

#include "utils/logger.h"
#include "utils/string_utils.hpp"
#include "text_cleaner/text_cleaner.hpp"
#include "text_normalizer/text_normalizer.hpp"
#include "g2p/EnEspeakG2P.hpp"
#include "g2p/LatinEspeakG2P.hpp"
#include "text_processor/jieba_processor.hpp"
#include "g2p/ZhJiebaG2P.hpp"
#include <mutex>

using namespace std;
using namespace utils;

// Impl
class KokoroFrontend::Impl {
public:
    Impl() = default;
    ~Impl() = default;

    bool init(AX_TTS_INIT_CONFIG* config) {
        if (config->espeak_data_path.empty()) {
            ALOGE("espeak_data_path is not set in config");
            return false;
        }

        espeak_data_path_ = config->espeak_data_path;
        jieba_dict_path_ = config->jieba_dict_path;

        if (jieba_dict_path_.empty()) {
            ALOGW("jieba_dict_path is empty; zh will be unavailable until set.");
        }

        // Preload g2p: English only; zh is loaded lazily.
        g2p_backends_.insert({"en", make_shared<EnEspeakG2P>(espeak_data_path_.c_str())});
        return true;
    }

    std::vector<int> run(const std::string& input_text, const std::string& language, const std::map<std::string, int>& vocab, int& err) {
        vector<int> tokens;

        if (language == "zh") {
            if (!ensure_zh_ready_()) {
                ALOGE("Zh processor not ready!");
                err = -1;
                return {};
            }
        }

        static const std::string kPhonemePrefix = "__PHONEMES__:";
        std::string phonemes;
        if (input_text.rfind(kPhonemePrefix, 0) == 0) {
            phonemes = input_text.substr(kPhonemePrefix.size());
        } else {
            // 1. Clean text (skip aggressive cleaning for Japanese to preserve fullwidth punctuation)
            auto cleaned_text = (language == "ja") ? input_text : text_cleaner_.run(input_text);
            // 2. Normalize text
            auto normalized_text = text_normalizer_.run(cleaned_text, language);
            // 3. G2P
            auto g2p = load_g2p_(language);
            phonemes = g2p->run(normalized_text, err);
        }

        ALOGD("input_text: %s", input_text.c_str());
        ALOGD("phonemes: %s", phonemes.c_str());
        
        // 4. Phonemes -> Tokens
        tokens.reserve(phonemes.length() + 2);
        tokens.emplace_back(0);

        vector<string> chars = utils::split_utf8(phonemes);
        for (const auto& c : chars) {
            if (vocab.count(c))
                tokens.emplace_back(vocab.at(c));
        }
        tokens.emplace_back(0);
        return tokens;
    }

private:
    shared_ptr<G2P> load_g2p_(const string& language) {
        if (g2p_backends_.find(language) == g2p_backends_.end()) {
            if (language == "zh") {
                if (!ensure_zh_ready_()) {
                    return nullptr;
                }
            }
            shared_ptr<G2P> new_g2p = make_shared<LatinEspeakG2P>(
                espeak_data_path_.c_str(),
                language
            );
            g2p_backends_.insert({language, new_g2p});
            return new_g2p;
        } else {
            return g2p_backends_.at(language);
        }
    }

    bool ensure_zh_ready_() {
        if (zh_ready_) {
            return true;
        }
        std::lock_guard<std::mutex> lock(zh_mutex_);
        if (zh_ready_) {
            return true;
        }
        if (jieba_dict_path_.empty()) {
            ALOGE("jieba_dict_path is empty, cannot init zh processor");
            return false;
        }
        zh_processor_ = make_shared<JiebaProcessor>();
        if (!zh_processor_->load_dict(jieba_dict_path_)) {
            ALOGE("Init zh_processor failed!");
            return false;
        }
        text_normalizer_.set_zh_processor(zh_processor_);
        g2p_backends_.insert({"zh", make_shared<ZhJiebaG2P>(zh_processor_)});
        zh_ready_ = true;
        return true;
    }

private:
    string espeak_data_path_;
    string jieba_dict_path_;
    map<string, shared_ptr<G2P>> g2p_backends_;  
    shared_ptr<TextProcessor> zh_processor_;
    TextCleaner text_cleaner_;
    TextNormalizer text_normalizer_;
    std::mutex zh_mutex_;
    bool zh_ready_ = false;
};

// KokoroFrontend
KokoroFrontend::KokoroFrontend():
    impl_(std::make_unique<Impl>()) {

}

KokoroFrontend::~KokoroFrontend() {
    impl_.reset();
}

bool KokoroFrontend::init(AX_TTS_INIT_CONFIG* config) {
    return impl_->init(config);
}

std::vector<int> KokoroFrontend::run(const std::string& input_text, const std::string& language, const std::map<std::string, int>& vocab, int& err) {
    return impl_->run(input_text, language, vocab, err);
}
