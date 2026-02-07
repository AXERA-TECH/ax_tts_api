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

using namespace std;
using namespace utils;

// Impl
class KokoroFrontend::Impl {
public:
    Impl() = default;
    ~Impl() = default;

    bool init(AX_TTS_INIT_CONFIG* config) {
        if (strlen(config->espeak_data_path) == 0) {
            ALOGE("espeak_data_path is not set in config");
            return false;
        }

        espeak_data_path_ = string(config->espeak_data_path);

        // Preload g2p: English
        g2p_backends_.insert({"en", make_shared<EnEspeakG2P>(espeak_data_path_.c_str())});
        // jieba processor for chinese
        zh_processor_ = make_shared<JiebaProcessor>(
            // TODO
        );

        return true;
    }

    std::vector<int> run(const std::string& input_text, const std::string& language, const std::map<std::string, int>& vocab, int& err) {
        vector<int> tokens;

        // 1. Clean text
        auto cleaned_text = text_cleaner_.run(input_text);
        // 2. Normalize text
        auto normalized_text = text_normalizer_.run(cleaned_text);
        // 3. G2P
        auto g2p = load_g2p_(language);
        auto phonemes = g2p->run(normalized_text, err);

        ALOGD("input_text: %s", input_text.c_str());
        ALOGD("cleaned_text: %s", cleaned_text.c_str());
        ALOGD("normalized_text: %s", normalized_text.c_str());
        ALOGD("phonemes: %s", phonemes.c_str());
        
        // 4. Phonemes -> Tokens
        tokens.reserve(phonemes.length() + 2);
        tokens.emplace_back(0);

        vector<string> chars = utils::split_utf8(phonemes);
        
        for (const auto& c : chars) {
            if (vocab.count(c))
                tokens.emplace_back(vocab.at(c));
            else
                ALOGW("Token: %s not found in vocab.", c.c_str());
        }
        return tokens;
    }

private:
    shared_ptr<G2P> load_g2p_(const string& language) {
        if (g2p_backends_.find(language) == g2p_backends_.end()) {
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

private:
    string espeak_data_path_;
    map<string, shared_ptr<G2P>> g2p_backends_;  
    shared_ptr<TextProcessor> zh_processor_;
    TextCleaner text_cleaner_;
    TextNormalizer text_normalizer_;
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