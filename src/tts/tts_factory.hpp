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
#include "tts/kokoro.hpp"
#include "tts/melotts.hpp"

class TTSFactory {
public:
    static TTSInterface* create(AX_TTS_TYPE_E tts_type, AX_TTS_INIT_CONFIG* tts_init_config) {
        TTSInterface* interface = nullptr;
        
        switch (tts_type)
        {
        case AX_KOKORO: {
            interface = new Kokoro();
            sprintf(tts_init_config->model_path, "%s/kokoro/", tts_init_config->model_path);
            break;
        }
        case AX_MELOTTS: {
            sprintf(tts_init_config->model_path, "%s/melotts/", tts_init_config->model_path);
            interface = new MeloTTS();
            break;
        }
        default:
            ALOGE("Unknown tts_type %d", tts_type);
            return nullptr;
        }

        if (!interface->init(tts_type, tts_init_config)) {
            ALOGE("Init tts failed!");
            delete interface;
            return nullptr;
        }

        return interface;
    }
};