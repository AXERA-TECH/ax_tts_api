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
#include "g2p/EspeakG2P.hpp"
#include "utils/logger.h"



// Other languages of espeak could be found here: https://espeak.sourceforge.net/languages.html
class LatinEspeakG2P : public G2P {
public:
    LatinEspeakG2P(const char* espeak_data_path = "./espeak-ng-data", const std::string& language = "en-us"):
        espeak_(espeak_data_path),
        language_(language) {
            
    }

    ~LatinEspeakG2P() = default;
    
    std::string get_language() const override { 
        return language_; 
    }

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
    bool british_;
    std::string language_;
};
