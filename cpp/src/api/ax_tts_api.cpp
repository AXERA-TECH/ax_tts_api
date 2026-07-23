/**************************************************************************************************
 *
 * Copyright (c) 2019-2026 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include "api/ax_tts_api.h"
#include "tts/kokoro.hpp"
#include "utils/logger.h"
#include <cstring>

#ifdef __cplusplus
extern "C" {
#endif

AX_TTS_API const char* AX_TTS_GetVersion(void) {
    return "1.0.0";
}

AX_TTS_API const char* AX_TTS_GetErrorString(AX_TTS_ERROR_E err) {
    switch (err) {
    case AX_TTS_OK:                    return "OK";
    case AX_TTS_ERR_NULL_HANDLE:       return "null handle";
    case AX_TTS_ERR_NULL_CONFIG:       return "null config";
    case AX_TTS_ERR_NULL_INPUT:        return "null or empty input text";
    case AX_TTS_ERR_INVALID_MODEL_PATH: return "invalid model path";
    case AX_TTS_ERR_MODEL_LOAD_FAILED:  return "model load failed";
    case AX_TTS_ERR_INFERENCE_FAILED:   return "inference failed";
    case AX_TTS_ERR_INVALID_LANGUAGE:   return "invalid language";
    case AX_TTS_ERR_OUT_OF_MEMORY:      return "out of memory";
    case AX_TTS_ERR_UNKNOWN_MODEL:      return "unknown model type";
    case AX_TTS_ERR_FRONTEND_FAILED:    return "frontend processing failed";
    case AX_TTS_ERR_INTERNAL:           return "internal error";
    default:                            return "unknown error";
    }
}

AX_TTS_API AX_TTS_ERROR_E AX_TTS_Init(AX_TTS_TYPE_E tts_type,
                                       const AX_TTS_INIT_CONFIG* init_config,
                                       AX_TTS_HANDLE* handle) {
    if (!init_config || !handle) {
        ALOGE("init_config or handle is NULL!");
        return AX_TTS_ERR_NULL_CONFIG;
    }
    
    if (init_config->model_path[0] == '\0') {
        ALOGE("model_path is empty!");
        return AX_TTS_ERR_INVALID_MODEL_PATH;
    }

    if (tts_type != AX_KOKORO) {
        ALOGE("Unknown tts_type %d", tts_type);
        return AX_TTS_ERR_UNKNOWN_MODEL;
    }

    // Make a mutable copy: Kokoro::init may modify model_path internally
    AX_TTS_INIT_CONFIG mutable_cfg = *init_config;

    Kokoro* kokoro = new Kokoro();
    if (!kokoro->init(tts_type, &mutable_cfg)) {
        ALOGE("Kokoro init failed!");
        delete kokoro;
        return AX_TTS_ERR_MODEL_LOAD_FAILED;
    }

    *handle = static_cast<AX_TTS_HANDLE>(kokoro);
    return AX_TTS_OK;
}

AX_TTS_API void AX_TTS_Uninit(AX_TTS_HANDLE handle) {
    if (handle) {
        Kokoro* kokoro = static_cast<Kokoro*>(handle);
        delete kokoro;
    }
}

AX_TTS_API AX_TTS_ERROR_E AX_TTS_Run(AX_TTS_HANDLE handle,
                                      const char* text,
                                      const AX_TTS_RUN_CONFIG* run_config,
                                      AX_TTS_AUDIO** audio) {
    if (!handle) {
        ALOGE("handle is NULL!");
        return AX_TTS_ERR_NULL_HANDLE;
    }

    if (!run_config) {
        ALOGE("run_config is NULL!");
        return AX_TTS_ERR_NULL_CONFIG;
    }

    if (!text || text[0] == '\0') {
        ALOGE("text is NULL or empty!");
        return AX_TTS_ERR_NULL_INPUT;
    }

    if (!audio) {
        ALOGE("audio is NULL!");
        return AX_TTS_ERR_NULL_CONFIG;
    }

    // Make a mutable copy for internal use (non-const interface)
    AX_TTS_RUN_CONFIG mutable_cfg = *run_config;

    Kokoro* kokoro = static_cast<Kokoro*>(handle);
    if (!kokoro->run(std::string(text), &mutable_cfg, audio)) {
        ALOGE("Kokoro run failed!");
        return AX_TTS_ERR_INFERENCE_FAILED;
    }

    return AX_TTS_OK;
}

#ifdef __cplusplus
}
#endif
