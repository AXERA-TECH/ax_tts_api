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

#include <vector>
#include <string>
#include <map>
#include <memory>
#include "api/ax_tts_api.h"

/*
 * Base frontend class for individual tts model. Implemented for every model for no known unified frontend for now.
 *
 * It basically does these things:
 * 1. Text clean, remove unprintable characters, full-width char to half-width char, other things that guarantee
 *      speekable texts.
 * 2. Text normalization, replace numbers, currency signs to specific language, for example, $100 -> one hundred dollars
 *      for English.
 * 3. Grapheme-to-Phoneme, also called G2P, converts written text into phonetic symbols, 
 *      typically using the International Phonetic Alphabet (IPA).
 * 4. Tone tuning, for some languages like Chinese, pronounciation could vary according to meaning of words, for example,
 *      奇(jī)数 vs. 奇(qí)迹
*/
class TTSFrontend {
public:
    TTSFrontend() = default;
    ~TTSFrontend() = default;

    virtual bool init(const AX_TTS_INIT_CONFIG* config) = 0;
    virtual std::vector<int> run(const std::string& input_text, const std::string& language, const std::map<std::string, int>& vocab, int& err) = 0;
};
