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
#include "utils/AudioFile.h"
#include "api/ax_tts_api.h"

static void test_input_text(AX_TTS_HANDLE handle, const std::string& input_text, const std::string& language) {
    // int err = 0;
    // auto phonemes = g2p.run(input_text, language, err);
    // if (err != 0) {
    //     ALOGE("Run g2p failed! err=%d", err);
    //     return;
    // }

    // printf("================================\n");
    // printf("test_input_text:\n");
    // printf("input text: %s\n", input_text.c_str());
    // printf("phonemes: %s\n", phonemes.c_str());
    // printf("\n");
}

static void test_en(AX_TTS_HANDLE handle) {
    std::string input_text("Hello, World!");
    
    AX_TTS_RUN_CONFIG run_config;
    run_config.fade_out = 0.3f;
    run_config.speed = 1.0f;
    run_config.sample_rate = 44100;
    run_config.language = "en";
    run_config.voice = "af_heart";

    AX_TTS_AUDIO* audio = NULL;
    AX_TTS_ERROR_E err = AX_TTS_Run(handle, input_text.c_str(), &run_config, &audio);
    if (err != AX_TTS_OK) {
        ALOGE("AX_TTS_Run failed! err=%d", err);
        free(audio);
        return;
    }

    std::string output_wav("test_en.wav");
    AudioFile<float> audio_file;
    std::vector<std::vector<float> > audio_samples{std::vector<float>(audio->data, audio->data + audio->num_samples)};
    audio_file.setAudioBuffer(audio_samples);
    audio_file.setSampleRate(run_config.sample_rate);
    if (!audio_file.save(output_wav)) {
        ALOGE("Save audio file failed!\n");
        return;
    }

    free(audio);

    printf("================================\n");
    printf("test_en:\n");
    printf("input text: %s\n", input_text.c_str());
    printf("output duration: %.2f seconds\n", audio_file.getNumSamplesPerChannel() * 1.0f / run_config.sample_rate);
    printf("output file: %s\n", output_wav.c_str());
    printf("\n");
}

int main(int argc, char** argv) {
    cmdline::parser cmd;
    cmd.add<std::string>("language", 'l', "Language, in ISO-639 format", false, "en");
    cmd.add<std::string>("text", 't', "Input text", false, "");
    cmd.parse_check(argc, argv);
    
    // 0. get app args, can be removed from user's app
    auto input_text = cmd.get<std::string>("text");
    auto language = cmd.get<std::string>("language");

    AX_TTS_INIT_CONFIG init_config;
    init_config.max_seq_len = 128;
#if defined(CHIP_AX650) || defined(CHIP_AX8850)    
    init_config.model_path = "models-ax650";
#else
    init_config.model_path = "models-ax630c";
#endif

    AX_TTS_HANDLE handle = NULL;
    AX_TTS_ERROR_E err = AX_TTS_Init(AX_MELOTTS, &init_config, &handle);
    if (err != AX_TTS_OK) {
        ALOGE("AX_TTS_Init failed! err=%d", err);
        return -1;
    }

    ALOGI("AX_TTS_Init success");

    test_en(handle);

    if (!input_text.empty() && !language.empty()) {
        test_input_text(handle, input_text, language);
    }

    AX_TTS_Uninit(handle);
    
    return 0;
}