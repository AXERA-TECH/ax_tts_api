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

#include <memory>
#include <map>

#include "utils/text_cleaner.hpp"
#include "utils/text_normalizer.hpp"
#include "utils/g2p/g2p.hpp"
#include "utils/g2p/EnEspeakG2P.hpp"
#include "utils/logger.h"
#include "utils/string_utils.hpp"

#define TTS_FRONTEND_MAX_LEN    64

typedef struct {
    char espeak_data_path[TTS_FRONTEND_MAX_LEN];

} TTSFrontendConfig;

class TTSFrontend {
public:
    TTSFrontend():
        inited_(false) {

    }
    ~TTSFrontend() = default;

    bool init(const TTSFrontendConfig& config) {
        g2p_ = std::make_unique<utils::EnEspeakG2P>();
        inited_ = true;
        return true;
    }

    std::vector<int> run(const std::string& input_text, const std::map<std::string, int>& vocab, int& err) {
        if (!inited_) {
            ALOGE("frontend is not inited, call init first!");
            err = -1;
            return std::vector<int>{};
        }

        auto cleaned_text = cleaner_.run(input_text);
        auto normalized_text = normalizer_.run(cleaned_text);
        auto phonemes = g2p_->run(normalized_text, err);

        ALOGD("input_text: %s", input_text.c_str());
        ALOGD("cleaned_text: %s", cleaned_text.c_str());
        ALOGD("normalized_text: %s", normalized_text.c_str());
        ALOGD("phonemes: %s", phonemes.c_str());

        std::vector<int> tokens;
        tokens.reserve(input_text.length() * 2);
        tokens.emplace_back(0);

        std::vector<std::string> chars = utils::split_utf8(phonemes);
        
        for (const auto& c : chars) {
            if (vocab.count(c)) {
                tokens.emplace_back(vocab.at(c));
            }
        }

        tokens.emplace_back(0);
        return tokens;
    }

private:
    bool inited_;
    utils::TextCleaner cleaner_;
    utils::TextNormalizer normalizer_;
    std::unique_ptr<utils::G2P> g2p_;
};