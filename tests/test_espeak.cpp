/**************************************************************************************************
 *
 * Copyright (c) 2019-2026 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include <stdio.h>

#include "utils/cmdline.hpp"
#include "utils/logger.h"
#include "utils/g2p/EspeakG2P.hpp"
#include "utils/g2p/EnEspeakG2P.hpp"
#include "utils/g2p/ZhEspeakG2P.hpp"


static void test_input_text(utils::EspeakG2P& g2p, const std::string& input_text, const std::string& language) {
    int err = 0;
    auto phonemes = g2p.run(input_text, language, err);
    if (err != 0) {
        ALOGE("Run g2p failed! err=%d", err);
        return;
    }

    printf("================================\n");
    printf("test_input_text:\n");
    printf("input text: %s\n", input_text.c_str());
    printf("phonemes: %s\n", phonemes.c_str());
    printf("\n");
}

static void test_en(utils::EnEspeakG2P& g2p) {
    int err = 0;
    std::string input_text("Hello, World!");
    auto phonemes = g2p.run(input_text, err);
    if (err != 0) {
        ALOGE("Run eng2p failed! err=%d", err);
        return;
    }

    printf("================================\n");
    printf("test_en:\n");
    printf("input text: %s\n", input_text.c_str());
    printf("phonemes: %s\n", phonemes.c_str());
    printf("\n");
}

static void test_zh(utils::ZhEspeakG2P& g2p) {
    int err = 0;
    std::string input_text("你好, 世界!");
    auto phonemes = g2p.run(input_text, err);
    if (err != 0) {
        ALOGE("Run zhg2p failed! err=%d", err);
        return;
    }

    printf("================================\n");
    printf("test_zh:\n");
    printf("input text: %s\n", input_text.c_str());
    printf("phonemes: %s\n", phonemes.c_str());
    printf("\n");
}

int main(int argc, char** argv) {
    cmdline::parser cmd;
    cmd.add<std::string>("language", 'l', "Language, in ISO-639 format", false, "");
    cmd.add<std::string>("text", 't', "Input text", false, "");
    cmd.parse_check(argc, argv);
    
    // 0. get app args, can be removed from user's app
    auto input_text = cmd.get<std::string>("text");
    auto language = cmd.get<std::string>("language");

    utils::EspeakG2P g2p;
    utils::EnEspeakG2P eng2p;
    utils::ZhEspeakG2P zhg2p;

    test_en(eng2p);
    test_zh(zhg2p);

    if (!input_text.empty() && !language.empty()) {
        test_input_text(g2p, input_text, language);
    }
    
    return 0;
}