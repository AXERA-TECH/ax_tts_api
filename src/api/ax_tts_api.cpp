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

/**
 * @brief Initialize the TTS system with specific configuration
 * 
 * Creates and initializes a new TTS context with the specified
 * model type, model path, and language. This function loads the appropriate
 * models, configures the generator, and prepares it for speech generation.
 * 
 * @param model_type Type of  model to use
 * @param model_path Directory path where model files are stored
 *                   Model files are expected to be in the format: *.axmodel
 * 
 * @return AX_TTS_HANDLE Opaque handle to the initialized  context,
 *         or NULL if initialization fails
 * 
 * @note The caller is responsible for calling AX_TTS_Uninit() to free
 *       resources when the handle is no longer needed.
 * @example
 *   // Initialize recognition with whisper tiny model
 *   AX_TTS_HANDLE handle = AX_TTS_Init(AX_KOKORO, "./models-ax650/");
 *   
 */
AX_TTS_API AX_TTS_HANDLE AX_ASR_Init(AX_TTS_TYPE_E tts_type, const char* model_path) {
    if (!model_path) {
        ALOGE("model_path is NULL!");
        return NULL;
    }

    TTSInterface* handle = TTSFactory::create(tts_type, std::string(model_path));
    if (!handle) {
        ALOGE("Create tts failed!");
        return NULL;
    }

    return static_cast<AX_TTS_HANDLE>(handle);
}

/**
 * @brief Deinitialize and release TTS resources
 * 
 * Cleans up all resources associated with the  context, including
 * unloading models, freeing memory, and releasing hardware resources.
 * 
 * @param handle  context handle obtained from AX_TTS_Init()
 * 
 * @warning After calling this function, the handle becomes invalid and
 *          should not be used in any subsequent API calls.
 */
AX_TTS_API void AX_TTS_Uninit(AX_TTS_HANDLE handle) {
    if (handle) {
        auto interface = static_cast<TTSInterface*>(handle);
        interface->uninit();
        delete interface;
    }
}

/**
 * @brief Perform speech generation and return dynamically allocated struct
 * 
 * @param handle context handle
 * @param text Text input to generate speech
 * @param tts_config Config of generation
 * @param audio Pointer to receive the allocated audio
 * 
 * @return int Status code (0 = success, <0 = error)
 * 
 * @note The returned audio is allocated with malloc() and must be freed
 *       by the caller using free() when no longer needed.
 */
AX_TTS_API int AX_TTS_Run(AX_TTS_HANDLE handle, 
                   const char* text, 
                   AX_TTS_CONFIG* tts_config,
                   AX_TTS_AUDIO** audio) {
    if (!handle) {
        ALOGE("handle is NULL!");
        return -1;
    }    

    return 0;
}

#ifdef __cplusplus
}
#endif                   