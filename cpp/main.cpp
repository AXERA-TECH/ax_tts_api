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
#include <unistd.h>
#include <limits.h>
#include <cstdlib>
#include <cstring>

#include "utils/cmdline.hpp"
#include "utils/logger.h"
#include "utils/AudioFile.h"
#include "api/ax_tts_api.h"

static std::string pick_path(const char* env_var, const char* default_rel) {
    const char* env = std::getenv(env_var);
    if (env && env[0] != '\0') {
        return std::string(env);
    }
    return std::string(default_rel);
}

int main(int argc, char** argv) {
    cmdline::parser cmd;
    cmd.add<std::string>("language", 'l', "Language, in ISO-639 format", true, "en");
    cmd.add<std::string>("text", 't', "Input text", true, "");
    cmd.add<std::string>("output", 'o', "Output wav path", true, "");
    cmd.add<std::string>("model_path", 0, "Model path (override default)", false, "");
    cmd.add<std::string>("espeak_data_path", 0, "Path to espeak-ng-data directory", false, "");
    cmd.add<std::string>("jieba_dict_path", 0, "Path to jieba dict directory", false, "");
    cmd.parse_check(argc, argv);
    
    // 0. get app args, can be removed from user's app
    auto input_text = cmd.get<std::string>("text");
    auto language = cmd.get<std::string>("language");
    auto output = cmd.get<std::string>("output");
    std::string voice;
    if (language == "ja")   voice = "jf_gongitsune";
    else if (language == "zh")   voice = "zf_xiaoxiao";
    else    voice = "af_heart";

    std::string model_path;
#if defined(CHIP_AX650) || defined(CHIP_AX8850)    
    model_path = "models-ax650";
#else
    model_path = "models-ax630c";
#endif
    if (!cmd.get<std::string>("model_path").empty()) {
        model_path = cmd.get<std::string>("model_path");
    }

    std::string espeak_data_path = cmd.get<std::string>("espeak_data_path");
    if (espeak_data_path.empty()) espeak_data_path = pick_path("AX_TTS_ESPEAK_DATA_PATH", "espeak-ng-data");
    std::string jieba_dict_path = cmd.get<std::string>("jieba_dict_path");
    if (jieba_dict_path.empty()) jieba_dict_path = pick_path("AX_TTS_JIEBA_DICT_PATH", "dict");

    AX_TTS_INIT_CONFIG init_config;
    init_config.max_seq_len = 96;
    init_config.model_path = model_path.c_str();
    init_config.espeak_data_path = espeak_data_path.c_str();
    init_config.jieba_dict_path = jieba_dict_path.c_str();

    // Check model files exist before init
    {
        bool found = false;
        for (const auto& sub : {"kokoro"}) {
            std::string probe = model_path + "/" + sub;
            if (access(probe.c_str(), F_OK) == 0) { found = true; break; }
        }
        if (!found) {
            fprintf(stderr, "ERROR: No model files under %s\n", model_path.c_str());
            fprintf(stderr, "Please run: bash download_models.sh\n");
            return -1;
        }
    }

    AX_TTS_HANDLE handle = NULL;
    AX_TTS_ERROR_E err = AX_TTS_Init(AX_KOKORO, &init_config, &handle);
    if (err != AX_TTS_OK) {
        ALOGE("AX_TTS_Init failed! err=%d", err);
        return -1;
    }

    AX_TTS_RUN_CONFIG run_config;
    run_config.fade_out = 0.3f;
    run_config.speed = 1.0f;
    run_config.sample_rate = 24000;
    run_config.language = language.c_str();
    run_config.voice = voice.c_str();

    AX_TTS_AUDIO* audio = NULL;
    err = AX_TTS_Run(handle, input_text.c_str(), &run_config, &audio);
    if (err != AX_TTS_OK) {
        ALOGE("AX_TTS_Run failed! err=%d", err);
        free(audio);
        return -1;
    }

    AudioFile<float> audio_file;
    std::vector<std::vector<float> > audio_samples{std::vector<float>(audio->data, audio->data + audio->num_samples)};
    audio_file.setAudioBuffer(audio_samples);
    audio_file.setSampleRate(run_config.sample_rate);
    if (!audio_file.save(output)) {
        ALOGE("Save audio file failed!\n");
        free(audio);
        return -1;
    }

    free(audio);

    printf("================================\n");
    printf("%s:\n", language.c_str());
    printf("input text: %s\n", input_text.c_str());
    printf("output duration: %.2f seconds\n", audio_file.getNumSamplesPerChannel() * 1.0f / run_config.sample_rate);
    printf("output file: %s\n", output.c_str());
    printf("\n");

    return 0;
}
