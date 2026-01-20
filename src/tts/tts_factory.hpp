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

#include <memory>
#include "tts/tts_interface.hpp"
#include "api/ax_tts_api.h"
#include "utils/logger.h"

class TTSFactory {
public:
    static TTSInterface* create(AX_TTS_TYPE_E tts_type, const std::string& model_path) {
        TTSInterface* interface = nullptr;
        
        std::string spec_model_path;
        spec_model_path.reserve(64);

        switch (tts_type)
        {
        case AX_KOKORO: {
            // interface = new Kokoro();
            // spec_model_path = model_path + "/kokoro";
            break;
        }
        default:
            ALOGE("Unknown tts_type %d", tts_type);
            return nullptr;
        }

        if (!interface->init(tts_type, spec_model_path)) {
            ALOGE("Init tts failed!");
            delete interface;
            return nullptr;
        }

        return interface;
    }
};