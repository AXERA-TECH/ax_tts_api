/**************************************************************************************************
 *
 * Copyright (c) 2019-2026 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include "text_normalizer/text_normalizer.hpp"
#include "text_processor/jieba_processor.hpp"
#include "utils/logger.h"


std::string TextNormalizer::run(const std::string& input_text, const std::string& language) {
    if (language == "zh") {
        if (!zh_processor_) {
            ALOGE("zh_processor is not set!");
            return "";
        }
        auto processed_text = zh_processor_->convert_numbers(input_text);
        return processed_text;
    } else {
        return input_text;
    }
}
