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
#include <regex>
#include <map>

#define _DEFAULT_MARKS  ";:,.!?¡¿—…\"«»“”(){}[]"

namespace utils {

typedef std::pair<std::string, std::string>     LineMarkPair;

// Following https://github.com/bootphon/phonemizer/blob/master/phonemizer/punctuation.py
// Split text according to punctuations, 
// Preserve punctuations and restore after, for backend like espeak strip punctuations.
// Example: 'hello, my world!' -> ['hello', 'my world'], [',', '!']
class Punctuator {
public:
    Punctuator(const std::string& marks = _DEFAULT_MARKS):
        marks_(marks) {

    }

    ~Punctuator() = default;

    static std::string default_marks() {
        return std::string(_DEFAULT_MARKS);
    }

    inline std::string get_marks() const {
        return marks_;
    }

    std::vector<LineMarkPair> run(const std::string& text) {
        return split_by_marks_(text, marks_);
    }

private:
    std::vector<LineMarkPair> split_by_marks_(
        const std::string& str, const std::string& delimiterChars) {
        
        std::vector<std::pair<std::string, std::string>> result;
        
        if (str.empty()) return result;
        if (delimiterChars.empty()) {
            result.emplace_back(str, "");
            return result;
        }
        
        // 转义特殊字符以构建安全的正则表达式
        std::string escaped;
        for (char c : delimiterChars) {
            if (c == '\\' || c == '^' || c == '$' || c == '.' || c == '|' ||
                c == '?' || c == '*' || c == '+' || c == '(' || c == ')' ||
                c == '[' || c == ']' || c == '{' || c == '}') {
                escaped += '\\';
            }
            escaped += c;
        }
        
        // 构建正则表达式：匹配分隔符或非分隔符序列
        std::string pattern = "([^" + escaped + "]+)|([" + escaped + "])";
        std::regex re(pattern);
        
        std::sregex_iterator it(str.begin(), str.end(), re);
        std::sregex_iterator end;
        
        while (it != end) {
            // 第一个捕获组：非分隔符内容
            if ((*it)[1].matched) {
                result.emplace_back((*it)[1].str(), "");
            }
            // 第二个捕获组：分隔符本身
            else if ((*it)[2].matched) {
                // 如果结果不为空且上一个token没有分隔符，则为上一个token设置分隔符
                if (!result.empty() && result.back().second.empty()) {
                    result.back().second = (*it)[2].str();
                } else {
                    // 否则作为独立的空token（开头就是分隔符的情况）
                    result.emplace_back("", (*it)[2].str());
                }
            }
            ++it;
        }
        
        return result;
    }

private:
    std::string marks_;
};

} // namespace utils