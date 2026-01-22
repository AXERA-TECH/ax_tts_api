/**************************************************************************************************
 *
 * Copyright (c) 2019-2026 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include "utils/text_cleaner.hpp"
#include "utils/cmdline.hpp"
#include <stdio.h>

static utils::TextCleaner g_cleaner;

static void test_fullwidth_to_halfwidth() {
    std::string input_text("【全角字符转半角字符。】");

    auto cleaned_text = g_cleaner.run(input_text);
    printf("================================\n");
    printf("test_fullwidth_to_halfwidth:\n");
    printf("input text: %s\n", input_text.c_str());
    printf("cleaned text: %s\n", cleaned_text.c_str());
    printf("\n");
}

static void test_input_text(const std::string& input_text) {
    auto cleaned_text = g_cleaner.run(input_text);
    printf("================================\n");
    printf("test_input_text:\n");
    printf("input text: %s\n", input_text.c_str());
    printf("cleaned text: %s\n", cleaned_text.c_str());
    printf("\n");
}

int main(int argc, char** argv) {
    cmdline::parser cmd;
    cmd.add<std::string>("text", 't', "Input text", false, "");
    cmd.parse_check(argc, argv);
    
    // 0. get app args, can be removed from user's app
    auto input_text = cmd.get<std::string>("text");
    
    test_fullwidth_to_halfwidth();

    if (!input_text.empty()) {
        test_input_text(input_text);
    }
    return 0;
}