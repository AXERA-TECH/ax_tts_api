/**************************************************************************************************
 *
 * Copyright (c) 2019-2026 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include "tts/melotts.hpp"
#include <map>
#include <fstream>
#include <stdio.h>
#include <algorithm>
#include <numeric>
#include <cstring>

#include "utils/logger.h"
#include "utils/memory_utils.hpp"
#include "ax_model_runner/ax_model_runner.hpp"

template <typename T>
void read_bin(const char* filename, std::vector<T>& data) {
    FILE* fp = fopen(filename, "rb");
    fread(data.data(), sizeof(T), data.size(), fp);
    fclose(fp);
}

class MeloTTS::Impl {
public:
    Impl() = default;

    ~Impl() {
        uninit();
    }

    bool init(AX_TTS_TYPE_E tts_type, AX_TTS_INIT_CONFIG* init_config) {
        max_phone_len_ = init_config->max_seq_len;
        max_dur_per_phone_ = 4;

        std::string model_path(init_config->model_path);

        if (!load_models_(model_path)) {
            ALOGE("Load models failed!");
            return false;
        }

        init_features_();

        return true;
    }

    void uninit() {
        encoder_.unload_model();
        decoder_.unload_model();
    }

    bool run(const std::string& text, AX_TTS_RUN_CONFIG* config, AX_TTS_AUDIO** audio) {
        if (!run_models_()) {
            ALOGE("Run models failed!");
            return false;
        }

        *audio = (AX_TTS_AUDIO*)malloc(sizeof(AX_TTS_AUDIO) + sizeof(float) * audio_data_.size());
        AX_TTS_AUDIO* audio_ptr = *audio;
        audio_ptr->channels = 1;
        audio_ptr->num_samples = audio_data_.size();
        audio_ptr->sample_rate = config->sample_rate;
        std::memcpy(audio_ptr->data, audio_data_.data(), sizeof(float) * audio_data_.size());

        return true;
    }

private:
    bool load_models_(const std::string& model_path) {
        std::string encoder_path = model_path + "/encoder-en.axmodel";
        std::string decoder_path = model_path + "/decoder-en.axmodel";

        int ret = encoder_.load_model(encoder_path.c_str());
        if (0 != ret) {
            ALOGE("Load encoder model failed! ret=0x%08x", ret);
            return false;
        }

        ret = decoder_.load_model(decoder_path.c_str());
        if (0 != ret) {
            ALOGE("Load decoder model failed! ret=0x%08x", ret);
            return false;
        }
        return true;
    }

    void init_features_() {
        phones_.resize(max_phone_len_); // [1, max_phone_len]
        tones_.resize(max_phone_len_); // [1, max_phone_len]
        lang_ids_.resize(max_phone_len_); // [1, max_phone_len]
        bert_.resize(1024 * max_phone_len_); // [1, 1024, max_phone_len]
        ja_bert_.resize(768 * max_phone_len_); // [1, 768, max_phone_len]

        m_p_.resize(192 * max_phone_len_); // [1, 192, max_phone_len]
        logs_p_.resize(192 * max_phone_len_); // [1, 192, max_phone_len]
        y_mask_.resize(max_phone_len_ * max_dur_per_phone_); // [1, 1, max_phone_len * max_dur_per_phone]
        attn_.resize(max_phone_len_ * max_dur_per_phone_ * max_phone_len_); // [1, 1, max_phone_len * max_dur_per_phone, max_phone_len]
        g_.resize(256); // [1, 256, 1]

        audio_len_ = decoder_.get_output_size(0) / sizeof(float);
        audio_data_.resize(audio_len_);
    }

    bool run_models_(void) {
        // m_p,
        // logs_p,
        // y_mask,
        // attn,
        // g,
        // noise_scale=torch.FloatTensor([0.667])
        read_bin<float>("ax_run_model/0/m_p.bin", m_p_);
        read_bin<float>("ax_run_model/0/logs_p.bin", logs_p_);
        read_bin<float>("ax_run_model/0/y_mask.bin", y_mask_);
        read_bin<float>("ax_run_model/0/attn.bin", attn_);
        read_bin<float>("ax_run_model/0/g.bin", g_);

        std::vector<void*> decoder_inputs {
            m_p_.data(),
            logs_p_.data(),
            y_mask_.data(),
            attn_.data(),
            g_.data(),
            (void*)&noise_scale_
        };

        int ret = decoder_.set_inputs(decoder_inputs);
        if (0 != ret) {
            ALOGE("Set decoder inputs failed! ret=0x%08x", ret);
            return false;
        }

        ret = decoder_.run();
        if (0 != ret) {
            ALOGE("Run decoder failed! ret=0x%08x", ret);
            return false;
        }

        ret = decoder_.get_output(0, audio_data_.data());
        if (0 != ret) {
            ALOGE("Get decoder output failed! ret=0x%08x", ret);
            return false;
        }

        return true;
    }

private:
    int max_phone_len_;
    int max_dur_per_phone_;
    AxModelRunner encoder_, decoder_;

    // encoder inputs
    std::vector<int> phones_;
    int phone_len_;
    int spk_id_;
    std::vector<int> tones_;
    std::vector<int> lang_ids_;
    std::vector<float> bert_;
    std::vector<float> ja_bert_;
    
    // decoder inputs
    std::vector<float> m_p_;
    std::vector<float> logs_p_;
    std::vector<float> y_mask_;
    std::vector<float> attn_;
    std::vector<float> g_;
    float noise_scale_ = 0.667f;

    // decoder output
    std::vector<float> audio_data_;
    int audio_len_;
};

MeloTTS::MeloTTS():
    impl_(std::make_unique<MeloTTS::Impl>()) {

}

MeloTTS::~MeloTTS() {
    uninit();
}

bool MeloTTS::init(AX_TTS_TYPE_E tts_type, AX_TTS_INIT_CONFIG* init_config) {
    return impl_->init(tts_type, init_config);
}

void MeloTTS::uninit(void) {
    impl_.reset();
}

bool MeloTTS::run(const std::string& text, AX_TTS_RUN_CONFIG* config, AX_TTS_AUDIO** audio) {
    return impl_->run(text, config, audio);
}