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

int main(int argc, char** argv) {
    cmdline::parser cmd;
    cmd.add<std::string>("language", 'l', "Language, in ISO-639 format", true, "en");
    cmd.add<std::string>("text", 't', "Input text", true, "");
    cmd.add<std::string>("output", 'o', "Output wav path", true, "");
    cmd.parse_check(argc, argv);
    
    // 0. get app args, can be removed from user's app
    auto input_text = cmd.get<std::string>("text");
    auto language = cmd.get<std::string>("language");
    auto output = cmd.get<std::string>("output");
    std::string voice;
    if (language == "zh")   voice = "zf_xiaoxiao";
    else    voice = "af_heart";

    AX_TTS_INIT_CONFIG init_config;
    init_config.max_seq_len = 96;
#if defined(CHIP_AX650) || defined(CHIP_AX8850)    
    snprintf(init_config.model_path, AX_TTS_MAX_STR_LEN, "%s", "models-ax650");
#else
    snprintf(init_config.model_path, AX_TTS_MAX_STR_LEN, "%s", "models-ax630c");
#endif
    snprintf(init_config.espeak_data_path, AX_TTS_MAX_STR_LEN, "%s", "espeak-ng-data");
    snprintf(init_config.jieba_dict_path, AX_TTS_MAX_STR_LEN, "%s", "dict");

    AX_TTS_HANDLE handle = AX_TTS_Init(AX_KOKORO, &init_config);
    if (!handle) {
        ALOGE("AX_TTS_Init failed!");
        return -1;
    }

    AX_TTS_RUN_CONFIG run_config;
    run_config.fade_out = 0.3f;
    run_config.speed = 1.0f;
    run_config.sample_rate = 24000;
    snprintf(run_config.language, AX_TTS_MAX_STR_LEN, "%s", language.c_str());
    snprintf(run_config.voice, AX_TTS_MAX_STR_LEN, "%s", voice.c_str());

    AX_TTS_AUDIO* audio = NULL;
    int ret = AX_TTS_Run(handle, 
                   input_text.c_str(), 
                   &run_config,
                   &audio); 
    if (ret != 0) {
        ALOGE("AX_TTS_Run failed!");
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
    printf("test_zh:\n");
    printf("input text: %s\n", input_text.c_str());
    printf("output duration: %.2f seconds\n", audio_file.getNumSamplesPerChannel() * 1.0f / run_config.sample_rate);
    printf("output file: %s\n", output.c_str());
    printf("\n");

    return 0;
}