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
#include "utils/logger.h"
#include "api/ax_tts_api.h"
#include "frontend/frontend_interface.hpp"
#include "frontend/kokoro_frontend.hpp"

class FrontendFactory {
public:
    static std::shared_ptr<TTSFrontend> create(AX_TTS_TYPE_E tts_type, AX_TTS_INIT_CONFIG* config) {
        std::shared_ptr<TTSFrontend> interface = nullptr;
        
        switch (tts_type)
        {
        case AX_KOKORO: {
            interface = std::make_shared<KokoroFrontend>();
            break;
        }
        case AX_MELOTTS: {
            
            break;
        }
        default:
            ALOGE("Unknown tts_type %d", tts_type);
            return nullptr;
        }

        if (!interface->init(config)) {
            ALOGE("Init frontend failed!");
            return nullptr;
        }

        return interface;
    }
};