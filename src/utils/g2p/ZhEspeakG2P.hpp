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

#include "utils/g2p/g2p.hpp"
#include "utils/g2p/EspeakG2P.hpp"
#include "utils/logger.h"

namespace utils {

class ZhEspeakG2P : public G2P {
public:
    ZhEspeakG2P(const char* espeak_data_path = "./espeak-ng-data"):
        espeak_(espeak_data_path) {

    }

    ~ZhEspeakG2P() = default;
    
    std::string get_language() const override { return "zh"; }
    std::string get_backend() const override { return "espeak"; }
    
    std::string run(const std::string& input_text, int& err) {
        std::string result = espeak_.run(input_text, get_language(), err);
        if (err != 0) {
            ALOGE("espeak run failed! err=%d", err);
            return std::string("");
        }
        return result;
    }

private:
    EspeakG2P espeak_;    
};

} // namespace utils