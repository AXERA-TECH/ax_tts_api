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

class G2P {
public:
    G2P() = default;
    virtual ~G2P() = default;

    virtual std::string get_language() const = 0;
    virtual std::string get_backend() const = 0;
    virtual std::string run(const std::string& input_text) = 0;
};

} // namespace utils