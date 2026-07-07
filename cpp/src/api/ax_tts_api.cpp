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
#include "tts/tts_factory.hpp"
#include "utils/logger.h"
#include "utils/AudioFile.h"

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
    
    if (!init_config->model_path || init_config->model_path[0] == '\0') {
        ALOGE("model_path is NULL or empty!");
        return AX_TTS_ERR_INVALID_MODEL_PATH;
    }

    TTSInterface* iface = TTSFactory::create(tts_type, init_config);
    if (!iface) {
        ALOGE("Create tts failed!");
        return AX_TTS_ERR_MODEL_LOAD_FAILED;
    }

    *handle = static_cast<AX_TTS_HANDLE>(iface);
    return AX_TTS_OK;
}

AX_TTS_API void AX_TTS_Uninit(AX_TTS_HANDLE handle) {
    if (handle) {
        auto interface = static_cast<TTSInterface*>(handle);
        interface->uninit();
        delete interface;
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

    auto interface = static_cast<TTSInterface*>(handle);
    if (!interface->run(std::string(text), run_config, audio)) {
        ALOGE("Run tts failed!");
        return AX_TTS_ERR_INFERENCE_FAILED;
    }

    return AX_TTS_OK;
}

#ifdef __cplusplus
}
#endif                   
