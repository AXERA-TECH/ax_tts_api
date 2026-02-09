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

class TextProcessor {
public:
    virtual ~TextProcessor() = default;
    virtual bool load_dict(const std::string& dict_path) = 0;
    // Return pairs of (word, pos_tag)
    virtual std::vector<std::pair<std::string, std::string>> cut(const std::string& text) = 0;
    virtual std::vector<std::string> word_to_pinyin(const std::string& word) = 0;
    virtual std::string convert_numbers(const std::string& text) = 0;
};
