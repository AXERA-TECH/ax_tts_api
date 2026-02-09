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
#include "text_processor/text_processor.hpp"


class TextNormalizer {
public:
    TextNormalizer() = default;
    ~TextNormalizer() = default;

    std::string run(const std::string& input_text, const std::string& language);

    void set_zh_processor(std::shared_ptr<TextProcessor> zh_processor) {
        zh_processor_ = zh_processor;
    }

private:
    std::shared_ptr<TextProcessor> zh_processor_;
};