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

#include "utils/g2p.hpp"

namespace utils {

class EnG2P : public G2P {
public:
    EnG2P() = default;
    ~EnG2P() = default;

    std::string get_language() const override { return "en"; }
};

} // namespace utils