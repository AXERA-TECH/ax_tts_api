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

#include "g2p/g2p.hpp"

namespace utils {

class ZhJiebaG2P : public G2P {
public:
    ZhJiebaG2P(const char* espeak_data_path = "./espeak-ng-data")

    }

    ~ZhJiebaG2P() = default;
    
    std::string get_language() const override { return "zh"; }
    std::string get_backend() const override { return "jieba"; }
    
    std::string run(const std::string& input_text, int& err); 
};

} // namespace utils