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

#include <regex>
#include <stdexcept>
#include "utils/cppjieba/Jieba.hpp"
#include "text_processor/text_processor.hpp"
#include "text_processor/PinyinFinder.h"
#include "utils/logger.h"
#include "utils/memory_utils.hpp"

class JiebaProcessor : public TextProcessor {
public:
    JiebaProcessor() = default;

    bool load_dict(const std::string& dict_path) {
        if (!utils::file_exist(dict_path)) {
            ALOGE("dict_path: %s not exist!", dict_path.c_str());
            return false;
        }

        std::string d = dict_path;
        if (!d.empty() && d.back() != '/') d += "/";

        std::string jieba_dict = d + "jieba.dict.utf8";
        std::string hmm_model = d + "hmm_model.utf8";
        std::string user_dict = d + "user.dict.utf8";
        std::string idf_path = d + "idf.utf8";
        std::string stop_word_path = d + "stop_words.utf8";
        std::string pinyin_char = d + "pinyin.txt";
        std::string pinyin_phrase = d + "pinyin_phrase.txt";
        std::string cmu_dict = d + "cmudict-0.7b/cmudict.dict";

        jieba = std::make_shared<cppjieba::Jieba>(
            jieba_dict, hmm_model, user_dict, idf_path, stop_word_path
        );

        finder = std::make_shared<PinyinFinder>();
        if (!finder->init(pinyin_char, pinyin_phrase)) {
            ALOGE("Failed to init PinyinFinder!");
            return false;
        }

        // TODO: load cmu_dict and apply eng2p

        return true;
    }

    std::vector<std::pair<std::string, std::string>> cut(const std::string& text) override {
        std::vector<std::pair<std::string, std::string>> result;
        std::vector<std::pair<std::string, std::string>> tag_words;
        
        // Use cppjieba Tagging
        jieba->Tag(text, tag_words);
        
        for (const auto& w : tag_words) {
            std::string word = w.first;
            std::string tag = w.second;
            
            // Ensure punctuation is 'x' (jieba might return 'w' for punct)
            if (tag == "w") tag = "x";
            
            // FIX: If tag is 'x' but contains Chinese characters, force it to a valid tag (e.g. 'n')
            // This prevents words like "我要" being tagged as 'x' and skipped by G2P.
            if (tag == "x") {
                bool has_cn = false;
                for (unsigned char c : word) {
                    if (c >= 0xE4 && c <= 0xE9) {
                        has_cn = true; 
                        break;
                    }
                }
                if (has_cn) tag = "n";
            }
            
            result.push_back({word, tag});
        }
        return result;
    }

    std::vector<std::string> word_to_pinyin(const std::string& word) override {
        std::vector<std::string> pinyins;
        if (finder) {
            finder->find_best_pinyin(word, pinyins);
        }
        return pinyins;
    }

    std::string convert_numbers(const std::string& text) override {
        // Regex to find numbers: integers, floats, and IP-like strings
        // Examples: 123, -123, 3.14, 192.168.0.1
        // Pattern: [-+]?\d+(?:\.\d+)*
        
        std::regex num_regex("[-+]?\\d+(?:\\.\\d+)*");
        std::string result;
        
        auto words_begin = std::sregex_iterator(text.begin(), text.end(), num_regex);
        auto words_end = std::sregex_iterator();
        
        size_t last_pos = 0;
        
        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            std::string match_str = match.str();
            
            // Append text before the number
            result += text.substr(last_pos, match.position() - last_pos);
            
            // Convert number to Chinese
            result += utils::NumberToChinese(match_str);
            
            last_pos = match.position() + match.length();
        }
        
        // Append remaining text
        if (last_pos < text.length()) {
            result += text.substr(last_pos);
        }
        
        return result;
    }

private:
    std::shared_ptr<cppjieba::Jieba> jieba;
    std::shared_ptr<PinyinFinder> finder;
};
