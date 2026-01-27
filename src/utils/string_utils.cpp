/**************************************************************************************************
 *
 * Copyright (c) 2019-2026 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include "utils/string_utils.hpp"
#include "utils/logger.h"

namespace utils {

std::vector<std::string> split_utf8(const std::string& utf8_text) {
    std::vector<std::string> chars;
    for (size_t i = 0; i < utf8_text.length();) {
        unsigned char c = static_cast<unsigned char>(utf8_text[i]);
        size_t char_len = 0;
        if (c < 0x80) char_len = 1;
        else if ((c & 0xE0) == 0xC0) char_len = 2;
        else if ((c & 0xF0) == 0xE0) char_len = 3;
        else if ((c & 0xF8) == 0xF0) char_len = 4;
        else char_len = 1; 

        if (i + char_len > utf8_text.length()) char_len = utf8_text.length() - i;
        
        chars.push_back(utf8_text.substr(i, char_len));
        i += char_len;
    }
    return chars;
}

}