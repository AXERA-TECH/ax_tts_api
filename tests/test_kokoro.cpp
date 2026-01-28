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
    run_config.sample_rate = 24000;
    snprintf(run_config.language, AX_TTS_MAX_STR_LEN, "%s", "en");
    snprintf(run_config.voice, AX_TTS_MAX_STR_LEN, "%s", "af_heart");

    AX_TTS_AUDIO* audio = NULL;
    int ret = AX_TTS_Run(handle, 
                   input_text.c_str(), 
                   &run_config,
                   &audio); 
    if (ret != 0) {
        ALOGE("AX_TTS_Run failed!");
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
    init_config.max_seq_len = 96;
    snprintf(init_config.model_path, AX_TTS_MAX_STR_LEN, "%s", "models-ax650/kokoro");
    snprintf(init_config.espeak_data_path, AX_TTS_MAX_STR_LEN, "%s", "espeak-ng-data");

    AX_TTS_HANDLE handle = AX_TTS_Init(AX_KOKORO, &init_config);
    if (!handle) {
        ALOGE("AX_TTS_Init failed!");
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