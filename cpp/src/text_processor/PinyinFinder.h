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

#include <string>
#include <vector>
#include <unordered_map>
#include "utils/string_utils.hpp"

class PinyinFinder {
public:
    using UnicodeCharT = char16_t;
    using UnicodeStr = std::u16string;

    PinyinFinder();
    ~PinyinFinder();

    bool init(const std::string& singleCharacterDictPath, const std::string& wordsDictPath);

    void find_best_pinyin(const std::string& phrasestr, std::vector<std::string>& pinyins);

private:
    std::unordered_map<UnicodeStr, std::string> word_pinyin_dict_;
    static const int kMaxChars = 8; // 最大匹配长度，通常不需要太大
};
