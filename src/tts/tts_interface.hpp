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
#include <vector>
#include "api/ax_tts_api.h"

class TTSInterface {
public:
    virtual ~TTSInterface() {}
    virtual bool init(AX_TTS_TYPE_E asr_type, const std::string& model_path) = 0;
    virtual void uninit(void) = 0;
    virtual bool run(const std::string& text, AX_TTS_CONFIG* config, AX_TTS_AUDIO** audio) = 0;
};