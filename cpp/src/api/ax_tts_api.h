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

/**
 * @file ax_tts_api.h
 * @brief Axera TTS C API — 文本转语音公共接口
 *
 * 典型调用流程：
 * @code
 *   AX_TTS_INIT_CONFIG cfg = {.max_seq_len = 96, .model_path = "models", ...};
 *   AX_TTS_HANDLE handle = NULL;
 *   AX_TTS_ERROR_E err = AX_TTS_Init(AX_KOKORO, &cfg, &handle);
 *   if (err != AX_TTS_OK) { ... }
 *
 *   AX_TTS_RUN_CONFIG run = {.speed = 1.0f, .voice = "af_heart", .language = "en", ...};
 *   AX_TTS_AUDIO* audio = NULL;
 *   err = AX_TTS_Run(handle, "Hello world", &run, &audio);
 *   // 使用 audio->data ...
 *   free(audio);
 *
 *   AX_TTS_Uninit(handle);
 * @endcode
 */

#define AX_TTS_API __attribute__((visibility("default")))

/** 支持的 TTS 模型类型 */
typedef enum {
    AX_KOKORO = 0,
    AX_MELOTTS
} AX_TTS_TYPE_E;

/** API 错误码。0 = 成功，负值 = 失败。 */
typedef enum {
    AX_TTS_OK                    =  0,
    AX_TTS_ERR_NULL_HANDLE       = -1,
    AX_TTS_ERR_NULL_CONFIG       = -2,
    AX_TTS_ERR_NULL_INPUT        = -3,
    AX_TTS_ERR_INVALID_MODEL_PATH = -4,
    AX_TTS_ERR_MODEL_LOAD_FAILED  = -5,
    AX_TTS_ERR_INFERENCE_FAILED   = -6,
    AX_TTS_ERR_INVALID_LANGUAGE   = -7,
    AX_TTS_ERR_OUT_OF_MEMORY      = -8,
    AX_TTS_ERR_UNKNOWN_MODEL      = -9,
    AX_TTS_ERR_FRONTEND_FAILED    = -10,
    AX_TTS_ERR_INTERNAL           = -100
} AX_TTS_ERROR_E;

/** 初始化配置。
 *  字符串字段为调用方所有的 C 字符串指针；AX_TTS_Init 内部会在需要时拷贝。
 *  调用方需保证指针在 AX_TTS_Init 返回前有效。 */
typedef struct {
    int max_seq_len;          /**< 最大序列长度，Kokoro 默认 96，MeloTTS 默认 128 */
    const char* model_path;   /**< 模型文件目录路径 */
    const char* espeak_data_path; /**< espeak-ng-data 目录路径 */
    const char* jieba_dict_path;  /**< jieba 词典目录路径（中文必需） */
} AX_TTS_INIT_CONFIG;

/** 推理运行配置。
 *  同 INIT_CONFIG，字符串字段为调用方所有的 C 字符串指针，
 *  调用方需保证指针在 AX_TTS_Run 返回前有效。 */
typedef struct {
    float speed;              /**< 语速，1.0 为正常 */
    float fade_out;           /**< 末尾淡出时长（秒），0 为不淡出 */
    int sample_rate;          /**< 输出采样率，Kokoro 建议 24000 */
    const char* voice;        /**< 音色名称，如 "af_heart", "zf_xiaoxiao" */
    const char* language;     /**< ISO-639 语言代码，如 "en", "zh", "ja" */
} AX_TTS_RUN_CONFIG;

/** 合成音频输出。
 *  data 为柔性数组，实际大小 = sizeof(AX_TTS_AUDIO) + num_samples * sizeof(float)。
 *  由 AX_TTS_Run 内部分配，调用方通过 free() 释放。 */
typedef struct {
    int sample_rate;
    int num_samples;
    int channels;
    float data[];
} AX_TTS_AUDIO;

/**
 * @brief TTS 上下文不透明句柄，封装引擎内部状态以保证 ABI 稳定。
 */
typedef void* AX_TTS_HANDLE;

/**
 * @brief 返回 API 版本字符串，格式 "major.minor.patch"。
 * @return 版本字符串，如 "1.0.0"。
 */
AX_TTS_API const char* AX_TTS_GetVersion(void);

/**
 * @brief 初始化 TTS 引擎。
 * 
 * @param tts_type    TTS 模型类型（AX_KOKORO 或 AX_MELOTTS）。
 * @param init_config 初始化配置指针，不可为 NULL。
 * @param handle      输出参数，成功时指向新创建的 TTS 句柄。
 * 
 * @return AX_TTS_OK                 成功
 * @return AX_TTS_ERR_NULL_CONFIG    init_config 为 NULL
 * @return AX_TTS_ERR_INVALID_MODEL_PATH  model_path 为空或无效
 * @return AX_TTS_ERR_UNKNOWN_MODEL  tts_type 未知
 * @return AX_TTS_ERR_MODEL_LOAD_FAILED  模型加载失败
 * @return AX_TTS_ERR_OUT_OF_MEMORY  内存不足
 * 
 * @note 调用方必须在不再使用时调用 AX_TTS_Uninit 释放资源。
 * @example
 *   AX_TTS_INIT_CONFIG cfg = {.max_seq_len = 96, .model_path = "./models-ax650",
 *                              .espeak_data_path = "espeak-ng-data"};
 *   AX_TTS_HANDLE h = NULL;
 *   if (AX_TTS_Init(AX_KOKORO, &cfg, &h) != AX_TTS_OK) { ... }
 */
AX_TTS_API AX_TTS_ERROR_E AX_TTS_Init(AX_TTS_TYPE_E tts_type,
                                       const AX_TTS_INIT_CONFIG* init_config,
                                       AX_TTS_HANDLE* handle);

/**
 * @brief 释放 TTS 上下文占用的所有资源。
 * 
 * @param handle  AX_TTS_Init 返回的句柄，传入 NULL 无操作。
 * 
 * @warning 调用后 handle 失效，不可再用于任何 API 调用。
 */
AX_TTS_API void AX_TTS_Uninit(AX_TTS_HANDLE handle);

/**
 * @brief 执行文本转语音合成。
 * 
 * @param handle     TTS 上下文句柄。
 * @param text       输入文本（UTF-8），不可为 NULL。
 * @param run_config 推理配置指针，不可为 NULL。
 * @param audio      输出参数，成功时指向动态分配的 AX_TTS_AUDIO 结构体。
 * 
 * @return AX_TTS_OK                 成功
 * @return AX_TTS_ERR_NULL_HANDLE    handle 为 NULL
 * @return AX_TTS_ERR_NULL_CONFIG    run_config 为 NULL
 * @return AX_TTS_ERR_NULL_INPUT     text 为 NULL 或空
 * @return AX_TTS_ERR_INVALID_LANGUAGE 不支持的语言
 * @return AX_TTS_ERR_INFERENCE_FAILED  推理执行失败
 * @return AX_TTS_ERR_FRONTEND_FAILED   前端处理（分词/G2P）失败
 * @return AX_TTS_ERR_OUT_OF_MEMORY  内存不足
 * 
 * @note audio 由 malloc 分配，调用方必须通过 free() 释放。
 */
AX_TTS_API AX_TTS_ERROR_E AX_TTS_Run(AX_TTS_HANDLE handle,
                                      const char* text,
                                      const AX_TTS_RUN_CONFIG* run_config,
                                      AX_TTS_AUDIO** audio);

#ifdef __cplusplus
}
#endif

#endif // _AX_TTS_API_H_
