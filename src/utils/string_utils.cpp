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

#include <sstream>
#include <algorithm>

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

// https://github.com/bootphon/phonemizer/blob/master/phonemizer/utils.py#L35
std::vector<std::string> str2list(const std::string& text, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(text);
    
    // getline extracts characters from tokenStream and stores them into token 
    // until the delimiter character is found.
    while (std::getline(tokenStream, token, delimiter)) {
        if (!token.empty())
            tokens.push_back(token);
    }

    if (tokens.empty()) {
        return std::vector<std::string>{text};
    }

    return tokens;
}

void replace_inplace(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
        
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

std::string strip(const std::string& s) {
    auto begin = 
        std::find_if_not(s.begin(), s.end(), ::isspace);
    auto end = 
        std::find_if_not(s.rbegin(), s.rend(), ::isspace).base();
    return (begin < end) ? std::string(begin, end) : "";
}

}