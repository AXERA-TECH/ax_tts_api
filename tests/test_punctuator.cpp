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

#include "utils/g2p/Punctuator.hpp"

static utils::Punctuator punc;

void test_en() {
    std::string text("Hello, World!");
    auto line_marks = punc.run(text);

    printf("test_en:\n");
    printf("Input text: %s\n", text.c_str());
    for (int i = 0; i < line_marks.size(); i++) {
        printf("Line[%d]: text: %s \t mark: %s\n", i, line_marks[i].first.c_str(), line_marks[i].second.c_str());
    }
}

int main(int argc, char** argv) {
    test_en();
    
    return 0;
}