/**************************************************************************************************
 *
 * Copyright (c) 2019-2026 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "ax_model_runner/ax_model_runner.hpp"
#include "g2p/EspeakG2P.hpp"
#include "text_processor/jieba_processor.hpp"
#include "utils/logger.h"

static std::string read_status_field(const std::string &key) {
    std::ifstream in("/proc/self/status");
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind(key, 0) == 0) {
            return line.substr(key.size());
        }
    }
    return std::string("N/A");
}

static void log_mem(const char *tag) {
    std::cout << "[MEM] " << tag
              << " VmRSS" << read_status_field("VmRSS:")
              << " VmSize" << read_status_field("VmSize:")
              << " VmPeak" << read_status_field("VmPeak:")
              << std::endl;
    std::cout.flush();
}

static bool load_vocab(const std::string &vocab_path, std::map<std::string, int> &vocab) {
    std::ifstream in(vocab_path);
    if (!in.is_open()) {
        ALOGE("Open vocab failed: %s", vocab_path.c_str());
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        size_t tab = line.find('\t');
        if (tab == std::string::npos) {
            continue;
        }
        std::string token = line.substr(0, tab);
        std::string id_str = line.substr(tab + 1);
        vocab[token] = std::stoi(id_str);
    }
    return true;
}

static std::string get_mode(int argc, char **argv) {
    if (argc >= 5) {
        return std::string(argv[4]);
    }
    return std::string("all");
}

int main(int argc, char **argv) {
    if (argc < 4) {
        std::cout << "Usage: " << argv[0]
                  << " <model_dir> <espeak_data_path> <jieba_dict_path> [mode]\n"
                  << "mode: all|jieba|espeak|vocab|model1|model2|model3\n";
        return 1;
    }

    std::string model_dir = argv[1];
    std::string espeak_path = argv[2];
    std::string jieba_path = argv[3];
    std::string mode = get_mode(argc, argv);

    log_mem("start");

    // 1) Load jieba dict
    if (mode == "all" || mode == "jieba") {
        log_mem("before_jieba_dict");
        JiebaProcessor jp;
        if (!jp.load_dict(jieba_path)) {
            ALOGE("Jieba load failed!");
            return 1;
        }
        log_mem("after_jieba_dict");
        if (mode == "jieba") {
            return 0;
        }
    }

    // 2) Init espeak
    if (mode == "all" || mode == "espeak") {
        log_mem("before_espeak_init");
        EspeakG2P espeak(espeak_path.c_str());
        int err = 0;
        espeak.run("hello", "en", err);
        log_mem("after_espeak_init");
        if (mode == "espeak") {
            return 0;
        }
    }

    // 3) Load vocab
    if (mode == "all" || mode == "vocab") {
        log_mem("before_vocab");
        std::map<std::string, int> vocab;
        if (!load_vocab(model_dir + "/kokoro/vocab.txt", vocab)) {
            return 1;
        }
        log_mem("after_vocab");
        if (mode == "vocab") {
            return 0;
        }
    }

    // 4) Load models one by one
    AxModelRunner model1;
    AxModelRunner model2;
    AxModelRunner model3;

    if (mode == "all" || mode == "model1") {
        log_mem("before_model1");
        if (model1.load_model((model_dir + "/kokoro/kokoro_part1_96.axmodel").c_str()) != 0) {
            ALOGE("Load model1 failed!");
            return 1;
        }
        log_mem("after_model1");
        if (mode == "model1") {
            return 0;
        }
    }

    if (mode == "all" || mode == "model2") {
        log_mem("before_model2");
        if (model2.load_model((model_dir + "/kokoro/kokoro_part2_96.axmodel").c_str()) != 0) {
            ALOGE("Load model2 failed!");
            return 1;
        }
        log_mem("after_model2");
        if (mode == "model2") {
            return 0;
        }
    }

    if (mode == "all" || mode == "model3") {
        log_mem("before_model3");
        if (model3.load_model((model_dir + "/kokoro/kokoro_part3_96.axmodel").c_str()) != 0) {
            ALOGE("Load model3 failed!");
            return 1;
        }
        log_mem("after_model3");
    }

    std::cout << "OK" << std::endl;
    return 0;
}
