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
#include "frontend/frontend_factory.hpp"

class TTSFactory {
public:
    static TTSInterface* create(AX_TTS_TYPE_E tts_type, const AX_TTS_INIT_CONFIG* tts_init_config) {
        TTSInterface* interface = nullptr;
        std::string model_path;
        
        switch (tts_type)
        {
        case AX_KOKORO: {
            interface = new Kokoro();
            model_path = std::string(tts_init_config->model_path) + "/kokoro/";
            break;
        }
        case AX_MELOTTS: {
            interface = new MeloTTS();
            model_path = std::string(tts_init_config->model_path) + "/melotts/";
            break;
        }
        default:
            ALOGE("Unknown tts_type %d", tts_type);
            return nullptr;
        }

        auto frontend = FrontendFactory::create(tts_type, tts_init_config);
        if (!frontend) {
            ALOGE("Create frontend failed!");
            delete interface;
            return nullptr;
        }

        interface->set_frontend(frontend);

        if (!interface->init(tts_type, model_path, tts_init_config)) {
            ALOGE("Init tts failed!");
            delete interface;
            return nullptr;
        }

        return interface;
    }
};
