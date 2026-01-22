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

namespace utils {

class TextNormalizer {
public:
    TextNormalizer() = default;
    ~TextNormalizer() = default;

    std::string run(const std::string& input_text) {
        return input_text;
    }
};

} // namespace utils