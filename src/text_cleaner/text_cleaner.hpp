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
#include <regex>
#include <string_view>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <cstdint>

namespace utils {

class TextCleaner {
public:
    TextCleaner() = default;
    ~TextCleaner() = default;

    std::string run(const std::string& input_text) {
        if (input_text.empty()) return "";
    
        // 全角转半角
        std::string text = _fullwidth_to_halfwidth(input_text);
        
        // 1. 替换连续空白字符
        std::regex ws_re(R"(\s+)");
        text = std::regex_replace(text, ws_re, " ");
        
        // 2. 去除首尾空白
        auto not_whitespace = [](unsigned char ch) {
            return !std::isspace(ch);
        };
        
        // 找到首尾非空白字符
        auto begin = std::find_if(text.begin(), text.end(), not_whitespace);
        if (begin == text.end()) {
            return "";  // 全是空白字符
        }
        auto end = std::find_if(text.rbegin(), text.rend(), not_whitespace).base();
        
        // 创建子字符串视图
        std::string trimmed(begin, end);
        
        // 3. 过滤字符
        std::string filtered;
        filtered.reserve(trimmed.length());
        
        // 遍历每个字符，只保留满足条件的字符：
        // 条件1：ord(char) >= 32  # ASCII值大于等于32（可打印字符和常见标点）
        //        - ASCII 0-31是控制字符（如换行符、回车符、制表符等）
        //        - ASCII 32是空格
        //        - ASCII 33-126是可打印字符
        // 条件2：or char in '\n\r\t'  # 或者字符是换行符、回车符、制表符
        //        虽然这些字符的ASCII值<32，但我们特别允许它们通过
        for (unsigned char ch : trimmed) {
            if (ch >= 32 || ch == '\n' || ch == '\r' || ch == '\t') {
                filtered.push_back(static_cast<char>(ch));
            }
        }

        return filtered;
    }


private:
    // 全角字符转半角
    std::string _fullwidth_to_halfwidth(const std::string& input) {
        std::string result;
        result.reserve(input.length());  // 预分配内存
        
        // 全角到半角的映射表（UTF-8编码）
        // 注意：全角字符通常是3字节的UTF-8编码
        static const std::unordered_map<std::string, std::string> full_to_half = {
            // 中文标点
            {"。", "."},    // 全角句号
            {"！", "!"},    // 全角感叹号
            {"？", "?"},    // 全角问号
            {"；", ";"},    // 全角分号
            {"，", ","},    // 全角逗号
            {"、", ","},    // 全角顿号（转换为逗号）
            {"：", ":"},    // 全角冒号
            {"＂", "\""},   // 全角双引号
            {"＇", "'"},    // 全角单引号
            {"（", "("},    // 全角左括号
            {"）", ")"},    // 全角右括号
            {"【", "["},    // 全角左方括号
            {"】", "]"},    // 全角右方括号
            {"《", "<"},    // 全角左书名号
            {"》", ">"},    // 全角右书名号
            
            // 全角空格（通常用于中文排版）
            {"　", " "},    // 全角空格转半角空格
            
            // 全角字母和数字（A-Z, a-z, 0-9）
            {"Ａ", "A"}, {"Ｂ", "B"}, {"Ｃ", "C"}, {"Ｄ", "D"}, {"Ｅ", "E"},
            {"Ｆ", "F"}, {"Ｇ", "G"}, {"Ｈ", "H"}, {"Ｉ", "I"}, {"Ｊ", "J"},
            {"Ｋ", "K"}, {"Ｌ", "L"}, {"Ｍ", "M"}, {"Ｎ", "N"}, {"Ｏ", "O"},
            {"Ｐ", "P"}, {"Ｑ", "Q"}, {"Ｒ", "R"}, {"Ｓ", "S"}, {"Ｔ", "T"},
            {"Ｕ", "U"}, {"Ｖ", "V"}, {"Ｗ", "W"}, {"Ｘ", "X"}, {"Ｙ", "Y"},
            {"Ｚ", "Z"},
            {"ａ", "a"}, {"ｂ", "b"}, {"ｃ", "c"}, {"ｄ", "d"}, {"ｅ", "e"},
            {"ｆ", "f"}, {"ｇ", "g"}, {"ｈ", "h"}, {"ｉ", "i"}, {"ｊ", "j"},
            {"ｋ", "k"}, {"ｌ", "l"}, {"ｍ", "m"}, {"ｎ", "n"}, {"ｏ", "o"},
            {"ｐ", "p"}, {"ｑ", "q"}, {"ｒ", "r"}, {"ｓ", "s"}, {"ｔ", "t"},
            {"ｕ", "u"}, {"ｖ", "v"}, {"ｗ", "w"}, {"ｘ", "x"}, {"ｙ", "y"},
            {"ｚ", "z"},
            {"０", "0"}, {"１", "1"}, {"２", "2"}, {"３", "3"}, {"４", "4"},
            {"５", "5"}, {"６", "6"}, {"７", "7"}, {"８", "8"}, {"９", "9"},
            
            // 更多全角符号
            {"＂", "\""},   // 全角双引号
            {"＇", "'"},    // 全角单引号
            {"＂", "\""},   // 全角双引号
            {"＇", "'"},    // 全角单引号
            {"～", "~"},    // 全角波浪号
            {"＠", "@"},    // 全角@
            {"＃", "#"},    // 全角#
            {"＄", "$"},    // 全角$
            {"％", "%"},    // 全角%
            {"＆", "&"},    // 全角&
            {"＊", "*"},    // 全角*
            {"＋", "+"},    // 全角+
            {"－", "-"},    // 全角-
            {"＝", "="},    // 全角=
            {"＼", "\\"},   // 全角反斜杠
            {"｜", "|"},    // 全角竖线
            {"｛", "{"},    // 全角左花括号
            {"｝", "}"},    // 全角右花括号
            {"＾", "^"},    // 全角^
            {"＿", "_"},    // 全角_
            {"｀", "`"},    // 全角`
            {"＜", "<"},    // 全角小于号
            {"＞", ">"},    // 全角大于号
        };
        
        size_t i = 0;
        while (i < input.length()) {
            // 检查是否是UTF-8多字节字符
            unsigned char ch = static_cast<unsigned char>(input[i]);
            
            if (ch < 128) {
                // ASCII字符，直接保留
                result.push_back(input[i]);
                i++;
            } else {
                // 可能是UTF-8字符，尝试匹配
                bool matched = false;
                
                // 检查最常见的3字节UTF-8字符（中文标点通常是3字节）
                if (i + 2 < input.length()) {
                    std::string utf8_char = input.substr(i, 3);
                    auto it = full_to_half.find(utf8_char);
                    if (it != full_to_half.end()) {
                        result += it->second;
                        i += 3;
                        matched = true;
                    }
                }
                
                // 如果没有匹配到，保持原字符
                if (!matched) {
                    // 复制整个UTF-8字符（可能是2-4字节）
                    int char_len = 0;
                    if ((ch & 0xF0) == 0xF0) char_len = 4;      // 4字节字符
                    else if ((ch & 0xE0) == 0xE0) char_len = 3; // 3字节字符
                    else if ((ch & 0xC0) == 0xC0) char_len = 2; // 2字节字符
                    else char_len = 1;                          // 不应该发生
                    
                    result += input.substr(i, char_len);
                    i += char_len;
                }
            }
        }
        
        return result;
    }
};

} // namespace utils