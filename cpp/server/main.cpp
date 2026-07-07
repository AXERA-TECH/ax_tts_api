/**************************************************************************************************
 *
 * Copyright (c) 2019-2026 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include "tts_server.hpp"
#include "utils/cmdline.hpp"
#include "utils/logger.h"

#include <stdio.h>

int main(int argc, char** argv) {
    cmdline::parser cmd;
    cmd.add<int>("port", 'p', "On which port to run the server", false, 8080);
#if defined(CHIP_AX650) || defined(CHIP_AX8850)
    cmd.add<std::string>("model_path", 'm', "model path which contains axmodel", false, "./models-ax650");
#else
    cmd.add<std::string>("model_path", 'm', "model path which contains axmodel", false, "./models-ax630c");
#endif
    cmd.add<std::string>("espeak_data_path", 0, "Path to espeak-ng-data directory", false, "");
    cmd.add<std::string>("jieba_dict_path", 0, "Path to jieba dict directory", false, "");
    cmd.parse_check(argc, argv);

    // 0. get app args, can be removed from user's app
    auto port = cmd.get<int>("port");
    auto model_path = cmd.get<std::string>("model_path");
    auto espeak_data_path = cmd.get<std::string>("espeak_data_path");
    auto jieba_dict_path = cmd.get<std::string>("jieba_dict_path");

    TTSServer server;
    if (!server.init(model_path, espeak_data_path, jieba_dict_path)) {
        ALOGE("Init server failed!");
        return -1;
    }

    server.start(port);

    return 0;
}
