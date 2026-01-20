/**************************************************************************************************
 *
 * Copyright (c) 2019-2026 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_TTS_API_H_
#define _AX_TTS_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#define AX_TTS_API __attribute__((visibility("default")))

// Supported TTS models
enum AX_TTS_TYPE_E {
    AX_KOKORO = 0,
};

// TTS config
typedef struct {
    int speed;
    int sample_rate;
    char voice[32];
    char language[32];
} AX_TTS_CONFIG;

// Speech audio
typedef struct {
    int sample_rate;
    int num_samples;
    int channels;
    float data[];
} AX_TTS_AUDIO;


/**
 * @brief Opaque handle type for TTS context
 * 
 * This handle encapsulates all internal state of the TTS system.
 * The actual implementation is hidden from C callers to maintain ABI stability.
 */
typedef void* AX_TTS_HANDLE;

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
AX_TTS_API AX_TTS_HANDLE AX_TTS_Init(AX_TTS_TYPE_E _type, const char* model_path);

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
AX_TTS_API void AX_TTS_Uninit(AX_TTS_HANDLE handle);

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
                   AX_TTS_AUDIO** audio);                

#ifdef __cplusplus
}
#endif

#endif // _AX_TTS_API_H_