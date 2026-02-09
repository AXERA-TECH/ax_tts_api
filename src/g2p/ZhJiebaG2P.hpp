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
#include <memory>
#include "g2p/g2p.hpp"
#include "text_processor/jieba_processor.hpp"


class ZhJiebaG2P : public G2P {
public:
    ZhJiebaG2P(std::shared_ptr<TextProcessor> processor):
        jieba_(processor) 
    {

    }

    ~ZhJiebaG2P() = default;
    
    std::string get_language() const override { return "zh"; }
    std::string get_backend() const override { return "jieba"; }
    
    std::string run(const std::string& input_text, int& err); 

private:
    // 静态工具方法
    static std::string retone(std::string p);
    static std::string py2ipa(const std::string& py);
    static std::string map_punctuation(std::string text);
    struct PinyinParts {
        std::string initial;
        std::string final;
        int tone;
    };
    static PinyinParts parse_pinyin(const std::string& pinyin);
    // IPA 转换相关的内部结构
    static std::string pinyin_to_ipa_convert(const std::string& pinyin);

private:
    std::shared_ptr<TextProcessor> jieba_;
};