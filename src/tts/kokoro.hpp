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

class Kokoro : public TTSInterface {
public:
    Kokoro();
    
    ~Kokoro();

    bool init(AX_TTS_TYPE_E tts_type, AX_TTS_INIT_CONFIG* init_config);
    void uninit(void);
    bool run(const std::string& text, AX_TTS_RUN_CONFIG* run_config, AX_TTS_AUDIO** audio);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};