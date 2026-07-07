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

#include "frontend/frontend_interface.hpp"

/*
 * Backend for different languages:
 * English: eSpeak
 * Chinese: Jieba
 * Others: eSpeak
*/
class KokoroFrontend : public TTSFrontend {
public:
    KokoroFrontend();
    ~KokoroFrontend();

    bool init(AX_TTS_INIT_CONFIG* config);
    std::vector<int> run(const std::string& input_text, const std::string& language, const std::map<std::string, int>& vocab, int& err);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};