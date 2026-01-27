/**************************************************************************************************
 *
 * Copyright (c) 2019-2026 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include <map>
#include <fstream>
#include <stdio.h>
#include <algorithm>
#include <numeric>

#include "tts/kokoro.hpp"
#include "tts/tts_frontend.hpp"
#include "utils/logger.h"
#include "utils/memory_utils.hpp"
#include "ax_model_runner/ax_model_runner.hpp"
#include "onnxruntime_cxx_api.h"
#include "utils/librosa/eigen3/Eigen/Dense"
#include "utils/librosa/librosa.h"

// Preprocess parameters
#define MAX_PHONEME_LENGTH   510 // max position embedding - 2
#define FIXED_SEQ_LEN    96
#define N_FFT  20
#define HOP_LENGTH  5
#define DOUBLE_INPUT_THRESHOLD  32  // 输入长度小于此值时复制一倍,适配短文本
#define STYLE_DIM   256

#define DEFAULT_SPEED   1.0f
#define DEFAULT_FADE_OUT    0.05f
#define DEFAULT_PAUSE   0.05f


using namespace std;

typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> DynMat;

typedef std::vector<std::vector<std::complex<float>>> FFT_RESULT;

// Helper functions
static std::vector<float> sigmoid(const std::vector<float>& x) {
    std::vector<float> result(x.size());
    for (int i = 0; i < x.size(); i++) {
        result[i] = 1.0f / (1.0f + expf(-x[i]));
    }
    return result;
}

template <typename T>
vector<size_t> argsort(const vector<T> &v, int len, bool reverse) {
    // initialize original index locations
    vector<size_t> idx(len);
    iota(idx.begin(), idx.end(), 0);

    // sort indexes based on comparing values in v
    // using std::stable_sort instead of std::sort
    // to avoid unnecessary index re-orderings
    // when v contains elements of equal values 
    if (!reverse)
        stable_sort(idx.begin(), idx.end(),
            [&v](size_t i1, size_t i2) {return v[i1] < v[i2];});
    else
        stable_sort(idx.begin(), idx.end(),
            [&v](size_t i1, size_t i2) {return v[i1] > v[i2];});

    return idx;
}

template <typename T>
vector<T> np_repeat(const vector<T> &v, const vector<int>& times) {
    vector<T> result;
    for (size_t i = 0; i < times.size(); i++) {
        for (int n = 0; n < times[i]; n++)
            result.push_back(v[i]);
    }
    return result;
}

template <typename T>
std::vector<T> linspace(T a, T b, size_t N) {
    T h = (b - a) / static_cast<T>(N-1);
    std::vector<T> xs(N);
    typename std::vector<T>::iterator x;
    T val;
    for (x = xs.begin(), val = a; x != xs.end(); ++x, val += h)
        *x = val;
    return xs;
}


class Kokoro::Impl {
public:
    Impl() = default;
    ~Impl() {
        uninit();
    }

    bool init(AX_TTS_TYPE_E tts_type, AX_TTS_INIT_CONFIG* init_config) {
        max_seq_len_ = init_config->max_seq_len;

        std::string model_path(init_config->model_path);
        std::string vocab_path = model_path + "/vocab.txt";
        voice_path_ = model_path + "/voices/";

        if (!init_config->espeak_data_path) {
            ALOGE("espeak_data_path is NULL!");
            return false;
        }

        if (!load_vocab_(vocab_path)) {
            return false;
        }

        TTSFrontendConfig frontend_config;
        sprintf(frontend_config.espeak_data_path, "%s", init_config->espeak_data_path);

        if (!frontend_.init(frontend_config)) {
            ALOGE("Init tts frontend failed!");
            return false;
        }

        if (!load_models_(model_path)) {
            ALOGE("Load models failed!");
            return false;
        }

        // Prepare model outputs
        duration_.resize(model1_.get_output_size(0) / sizeof(float));
        d_.resize(model1_.get_output_size(1) / sizeof(float));

        // F0_pred, N_pred, asr = outputs2
        F0_pred_.resize(model2_.get_output_size(0) / sizeof(float));
        N_pred_.resize(model2_.get_output_size(1) / sizeof(float));
        asr_.resize(model2_.get_output_size(2) / sizeof(float));

        x_.resize(model3_.get_output_size(0) / sizeof(float));

        duration_shape_ = model1_.get_output_shape(0);
        d_shape_ = model1_.get_output_shape(1);

        F0_pred_shape_ = model2_.get_output_shape(0);

        x_shape_ = model3_.get_output_shape(0);

        return true;
    }

    void uninit(void) {
        model1_.unload_model();
        model2_.unload_model();
        model3_.unload_model();
        model4_.release();
    }

    bool run(const std::string& text, AX_TTS_RUN_CONFIG* run_config, AX_TTS_AUDIO** audio) {
        if (!run_config->voice) {
            ALOGE("voice is not set");
            return false;
        }

        std::string voice_name(run_config->voice);
        if (voice_name != voice_name_) {
            // Reload voice tensor if voice name is changed
            if (!get_voice_style_(voice_path_, voice_name)) {
                ALOGE("Load voice failed!");
                return false;
            }
            voice_name_ = voice_name;
        }

        int err = 0;
        auto input_ids = frontend_.run(text, vocab_, err);
        if (err != 0) {
            return false;
        }

        // get voice
        auto ref_s = load_voice_embedding_(input_ids.size());

        std::vector<float> audio_data;
        if (!run_models_(input_ids, ref_s, run_config->speed, run_config->fade_out, run_config->sample_rate, audio_data)) {
            ALOGE("Run models failed!");
            return false;
        }

        *audio = (AX_TTS_AUDIO*)malloc(sizeof(AX_TTS_AUDIO) + sizeof(float) * audio_data.size());
        AX_TTS_AUDIO* audio_ptr = *audio;
        audio_ptr->channels = 1;
        audio_ptr->num_samples = audio_data.size();
        audio_ptr->sample_rate = run_config->sample_rate;
        std::memcpy(audio_ptr->data, audio_data.data(), sizeof(float) * audio_data.size());

        return true;
    }

private:
    bool load_vocab_(const std::string& vocab_path) {
        if (!utils::file_exist(vocab_path)) {
            ALOGE("vocab path(%s) not exist!", vocab_path.c_str());
            return false;
        }

        std::ifstream in(vocab_path);
        if (in.is_open()) {
            std::string line;
            while (std::getline(in, line)) {
                // Expected format: token<TAB>id
                size_t tab = line.find('\t');
                if (tab != std::string::npos) {
                    std::string token = line.substr(0, tab);
                    std::string id_str = line.substr(tab + 1);
                    // Unescape token if needed (\n, \r, \t)
                    size_t pos = 0;
                    while((pos = token.find("\\n", pos)) != std::string::npos) { token.replace(pos, 2, "\n"); pos += 1; }
                    pos = 0;
                    while((pos = token.find("\\r", pos)) != std::string::npos) { token.replace(pos, 2, "\r"); pos += 1; }
                    pos = 0;
                    while((pos = token.find("\\t", pos)) != std::string::npos) { token.replace(pos, 2, "\t"); pos += 1; }
                    
                    vocab_[token] = std::stoi(id_str);
                }
            }
        } else {
            ALOGE("Failed to open vocab file %s", vocab_path.c_str());
            return false;
        }

        return true;
    }

    bool load_models_(const std::string& model_path) {
        std::string model1_path = model_path + "/kokoro_part1_96.axmodel";
        std::string model2_path = model_path + "/kokoro_part2_96.axmodel";
        std::string model3_path = model_path + "/kokoro_part3_96.axmodel";
        std::string model4_path = model_path + "/model4_har_sim.onnx";

        int ret = 0;
        ret = model1_.load_model(model1_path.c_str());
        if (ret != 0) {
            ALOGE("Load model1 from %s failed! ret=0x%x", model1_path.c_str(), ret);
            return false;
        }

        ret = model2_.load_model(model2_path.c_str());
        if (ret != 0) {
            ALOGE("Load model2 from %s failed! ret=0x%x", model2_path.c_str(), ret);
            return false;
        }

        ret = model3_.load_model(model3_path.c_str());
        if (ret != 0) {
            ALOGE("Load model3 from %s failed! ret=0x%x", model3_path.c_str(), ret);
            return false;
        }

        if (!utils::file_exist(model4_path)) {
            ALOGE("model4 %s not exist", model4_path.c_str());
            return false;
        }

        env_ = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "Kokoro");
        // Initialize session options
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        model4_ = Ort::Session(env_, model4_path.c_str(), session_options);

        // Prepare model outputs
        duration_.resize(model1_.get_output_size(0) / sizeof(float));
        d_.resize(model1_.get_output_size(1) / sizeof(float));

        // F0_pred, N_pred, asr = outputs2
        F0_pred_.resize(model2_.get_output_size(0) / sizeof(float));
        N_pred_.resize(model2_.get_output_size(1) / sizeof(float));
        asr_.resize(model2_.get_output_size(2) / sizeof(float));

        x_.resize(model3_.get_output_size(0) / sizeof(float));

        duration_shape_ = model1_.get_output_shape(0);
        d_shape_ = model1_.get_output_shape(1);

        F0_pred_shape_ = model2_.get_output_shape(0);

        x_shape_ = model3_.get_output_shape(0);
        return true;
    }

    bool get_voice_style_(const std::string& voices_path, const std::string& voice_name) {
        // 打开文件（二进制模式）
        std::string voice_bin_path = voices_path + "/" + voice_name + ".bin";
        if (!utils::file_exist(voice_bin_path)) {
            ALOGE("voice path %s not exist", voice_bin_path.c_str());
            return false;
        }

        voice_tensor_.resize(MAX_PHONEME_LENGTH * STYLE_DIM);
        std::vector<char> raw_data;
        if (!utils::read_file(voice_bin_path, raw_data)) {
            ALOGE("Read file %s failed!", voice_bin_path.c_str());
            return false;
        }

        if (raw_data.size() / 4 != voice_tensor_.size()) {
            ALOGE("File size not equal to %d*%d", MAX_PHONEME_LENGTH, STYLE_DIM);
            return false;
        }
        
        std::memcpy(voice_tensor_.data(), raw_data.data(), raw_data.size());
        return true;
    }

    std::vector<float> load_voice_embedding_(int phoneme_len) {
        phoneme_len = std::max(phoneme_len, 0);
        std::vector<float> ref_s(STYLE_DIM);
        if (phoneme_len < MAX_PHONEME_LENGTH) {
            ref_s.assign(voice_tensor_.begin() + phoneme_len * STYLE_DIM, voice_tensor_.begin() + (phoneme_len + 1) * STYLE_DIM);
        } else {
            int idx = MAX_PHONEME_LENGTH / 2;
            ref_s.assign(voice_tensor_.begin() + idx * STYLE_DIM, voice_tensor_.begin() + (idx + 1) * STYLE_DIM);
        }
        return ref_s;
    }

    bool run_models_(
        std::vector<int>& input_ids,
        const std::vector<float>& ref_s,
        float speed,
        float fade_out,
        int sample_rate,
        std::vector<float>& audio
    ) {
        int actual_len = input_ids.size();
        // 填充到固定长度
        int padding_len = max_seq_len_ - actual_len;
        if (padding_len > 0) {
            std::vector<int> padding(padding_len, 0);
            input_ids.insert(input_ids.end(), padding.begin(), padding.end());
        }

        int fade_samples = 0;
        if (fade_out > 0) {
            fade_samples = int(sample_rate * fade_out);
        }

        int actual_content_frames;
        int total_frames;
        if (!inference_single_chunk_(input_ids, ref_s, actual_len, speed, audio, actual_content_frames, total_frames)) {
            return false;
        }

        trim_audio_by_content_(
            audio, actual_content_frames, total_frames, actual_len
        );

        if (fade_samples > 0)
            apply_fade_out_(audio, fade_samples);

        return true;
    }

    bool inference_single_chunk_(
        std::vector<int>& input_ids,
        const std::vector<float>& ref_s,
        int actual_len,
        float speed,
        std::vector<float>& audio,
        int& actual_content_frames,
        int& total_frames
    ) {
        int ret = 0;
        // Prepare inputs
        bool is_doubled = false;
        int original_actual_len = actual_len;

        prepare_input_ids_(input_ids, actual_len, is_doubled);

        std::vector<int> input_lengths;
        std::vector<uint8_t> text_mask;
        compute_external_preprocessing_(input_ids, actual_len, input_lengths, text_mask);

        // outputs1 = self.session1.run(None, {'input_ids': input_ids.astype(np.int32), 'ref_s': ref_s, 'text_mask': text_mask.astype(np.uint8)})
        std::vector<void*> model1_inputs{(void*)input_ids.data(), (void*)ref_s.data(), (void*)text_mask.data()};
        std::vector<void*> model1_outputs{(void*)duration_.data(), (void*)d_.data()};

        // printf("run model 1\n");
        model1_.set_inputs(model1_inputs);
        ret = model1_.run();
        if (0 != ret) {
            ALOGE("Run model1 failed! ret=0x%x", ret);
            return false;
        }
        model1_.get_outputs(model1_outputs);

        // 处理duration并对齐
        std::vector<int> pred_dur;
        process_duration_(duration_, actual_len, speed, pred_dur, total_frames);
        auto pred_aln_trg = create_alignment_matrix_(pred_dur, total_frames);

        // Model2: 预测F0和ASR特征
        // d_transposed = np.transpose(d, (0, 2, 1))
        // en = d_transposed @ pred_aln_trg
        DynMat M_d = Eigen::Map<DynMat>(
            d_.data(), 
            d_shape_[1],  // 96
            d_shape_[2]   // 640
        );
        
        DynMat M_pred_aln_trg = Eigen::Map<DynMat>(
            pred_aln_trg.data(), 
            max_seq_len_,  // 96
            total_frames   // 192
        );

        DynMat M_en = M_d.transpose() * M_pred_aln_trg;
        std::vector<float> en(M_en.size());
        std::memcpy(en.data(), M_en.data(), M_en.size() * sizeof(float));

        std::vector<float> text_mask_float;
        std::transform(text_mask.begin(), text_mask.end(),
                    std::back_inserter(text_mask_float),
                    [](uint8_t i) { return static_cast<float>(i); });

        // F0_pred, N_pred, asr = outputs2
        std::vector<void*> model2_inputs{
            (void*)en.data(), 
            (void*)ref_s.data(), 
            (void*)input_ids.data(),
            (void*)text_mask_float.data(), 
            (void*)pred_aln_trg.data()
        };
        std::vector<void*> model2_outputs{
            (void*)F0_pred_.data(),
            (void*)N_pred_.data(), 
            (void*)asr_.data()
        };

        model2_.set_inputs(model2_inputs);
        ret = model2_.run();
        if (0 != ret) {
            ALOGE("Run model2 failed! ret=0x%x", ret);
            return false;
        }
        model2_.get_outputs(model2_outputs);

        std::vector<float> har;
        compute_har_onnx_(F0_pred_, har);

        std::vector<void*> model3_inputs{
            (void*)asr_.data(), 
            (void*)F0_pred_.data(), 
            (void*)N_pred_.data(),
            (void*)ref_s.data(), 
            (void*)har.data()
        };

        // printf("run model 3\n");
        model3_.set_inputs(model3_inputs);
        ret = model3_.run();
        if (0 != ret) {
            ALOGE("Run model3 failed! ret=0x%x", ret);
            return false;
        }
        model3_.get_output(0, x_.data());

        // 转换为音频
        postprocess_x_to_audio_(x_, audio);
        
        if (is_doubled) {
            actual_content_frames = std::accumulate(pred_dur.begin(), pred_dur.begin() + original_actual_len, 0);   
            size_t audio_len = audio.size();
            audio.erase(audio.begin() + audio_len / 2, audio.end());
            total_frames = total_frames / 2;
        } else {
            actual_content_frames = std::accumulate(pred_dur.begin(), pred_dur.begin() + actual_len, 0);
        }

        return true;
    }

    void trim_audio_by_content_(std::vector<float>& audio, int actual_content_frames, int total_frames, int actual_len) {
        // 根据实际内容比例裁剪音频
        int padding_len = max_seq_len_ - actual_len;
        if (padding_len > 0) {
            float content_ratio = actual_content_frames * 1.0f / total_frames;
            int audio_len_to_keep = int(audio.size() * content_ratio);
            audio.resize(audio_len_to_keep);
        }
    }

    void apply_fade_out_(std::vector<float>& audio, int fade_samples) {
        // 末尾淡出音频
        if (audio.size() <= fade_samples || fade_samples <= 0)
            return;

        std::vector<float> fade_out = linspace(1.0f, 0.0f, fade_samples);
        // audio_faded = audio.copy()
        // audio_faded[-fade_samples:] *= fade_out
        // return audio_faded
        for (int i = 0; i < fade_samples; i++) {
            audio[i - fade_samples + audio.size()] *= fade_out[i];
        }
    }

    void prepare_input_ids_(std::vector<int>& input_ids, int& actual_len, bool& is_doubled) {
        // 准备输入ID，对短输入进行复制处理
        is_doubled = false;
        int original_actual_len = actual_len;

        // printf("actual_len 3: %d\n", actual_len);
        if (actual_len <= DOUBLE_INPUT_THRESHOLD) {
            // printf("doubled!\n");
            is_doubled = true;
            // valid_content = input_ids[:, :actual_len]
            std::vector<int> valid_content(input_ids.begin(), input_ids.begin() + actual_len);
            // input_ids_doubled = np.concatenate([valid_content, valid_content], axis=1)
            std::vector<int> input_ids_doubled;
            // input_ids_doubled.reserve(2 * actual_len);
            input_ids_doubled.insert(input_ids_doubled.end(), valid_content.begin(), valid_content.end());
            input_ids_doubled.insert(input_ids_doubled.end(), valid_content.begin(), valid_content.end());
            
            // padding_len = self.max_seq_len_ - input_ids_doubled.shape[1]
            int padding_len = max_seq_len_ - 2 * actual_len;
            // printf("padding_len: %d\n", padding_len);
            if (padding_len > 0) {
                // input_ids = np.concatenate([input_ids_doubled, np.zeros((1, padding_len), dtype=input_ids.dtype)], axis=1)
                std::vector<int> padding(padding_len, 0);
                input_ids_doubled.insert(input_ids_doubled.end(), padding.begin(), padding.end());
            }
            else {
                // input_ids = input_ids_doubled[:, :self.max_seq_len_]
                input_ids_doubled.resize(max_seq_len_);
            }

            // save_file(input_ids_doubled, "input_ids2_1.bin");
                
            input_ids = input_ids_doubled;
            actual_len = std::min(original_actual_len * 2, max_seq_len_);
        }
    }

    void compute_external_preprocessing_(const std::vector<int>& input_ids, int actual_len, std::vector<int>& input_lengths, std::vector<uint8_t>& text_mask) {
        // 计算输入预处理：长度和mask
        // input_lengths = np.full((input_ids.shape[0],), actual_len, dtype=np.int64)
        input_lengths = std::vector<int>{actual_len};
        // text_mask = np.arange(self.max_seq_len_)[np.newaxis, :] >= input_lengths[:, np.newaxis]
        text_mask.resize(max_seq_len_);
        for (int i = 0; i < max_seq_len_; i++) {
            text_mask[i] = (i >= actual_len) ? 1 : 0;
        }
    }

    void process_duration_(const std::vector<float>& duration, int actual_len, float speed, std::vector<int>& pred_dur, int& total_frames) {
        // """处理duration预测，调整到固定帧数"""
        // duration_processed = 1.0 / (1.0 + np.exp(-duration))
        // duration_processed = duration_processed.sum(axis=-1) / speed
        // pred_dur_original = np.round(duration_processed).clip(min=1).astype(np.int64).squeeze()
        std::vector<int> pred_dur_original(actual_len, 0);
        std::vector<float> duration_processed = sigmoid(duration);
        for (int i = 0; i < actual_len; i++) {
            float sum = 0;

            // duration shape: [1, 96, 50]
            for (int n = 0; n < duration_shape_[2]; n++) {
                sum += duration_processed[i * duration_shape_[2] + n];
            }
            sum /= speed;

            pred_dur_original[i] = int(std::max(1.f, roundf(sum)));
        }

        // # 分离实际内容和padding
        // pred_dur_actual = pred_dur_original[:actual_len]
        // pred_dur_padding = np.zeros(self.max_seq_len_ - actual_len, dtype=np.int64)
        // pred_dur = np.concatenate([pred_dur_actual, pred_dur_padding])
        std::vector<int> pred_dur_padding(max_seq_len_ - actual_len, 0);
        pred_dur = pred_dur_original;
        pred_dur.insert(pred_dur.end(), pred_dur_padding.begin(), pred_dur_padding.end());

        
        // # 调整实际内容部分，只处理长度超出情况
        // fixed_total_frames = self.max_seq_len_ * 2
        // diff = fixed_total_frames - pred_dur[:actual_len].sum()
        
        // if diff < 0:
        //     # 减少帧数
        //     indices = np.argsort(pred_dur[:actual_len])[::-1]
        //     decreased = 0
        //     for idx in indices:
        //         if pred_dur[idx] > 1 and decreased < abs(diff):
        //             pred_dur[idx] -= 1
        //             decreased += 1
        //         if decreased >= abs(diff):
        //             break

        // 调整实际内容部分，只处理长度超出情况
        int fixed_total_frames = max_seq_len_ * 2;
        int actual_frames = std::accumulate(pred_dur.begin(), pred_dur.begin() + actual_len, 0);
        int diff = fixed_total_frames - actual_frames;

        if (diff < 0) {
            // 减少帧数
            auto indices = argsort(pred_dur, actual_len, true);
            int decreased = 0;
            for (auto idx : indices) {
                if (pred_dur[idx] > 1 && decreased < std::abs(diff)) {
                    pred_dur[idx]--;
                    decreased++;
                }
                if (decreased >= std::abs(diff))
                    break;
            }
        }
        
        // # 将剩余帧数分配到padding部分
        // remaining_frames = fixed_total_frames - pred_dur[:actual_len].sum()
        // padding_len = self.max_seq_len_ - actual_len
        // if remaining_frames > 0 and padding_len > 0:
        //     frames_per_padding = remaining_frames // padding_len
        //     remainder = remaining_frames % padding_len
        //     pred_dur[actual_len:] = frames_per_padding
        //     if remainder > 0:
        //         pred_dur[actual_len:actual_len+remainder] += 1

        actual_frames = std::accumulate(pred_dur.begin(), pred_dur.begin() + actual_len, 0);
        int remaining_frames = fixed_total_frames - actual_frames;
        int padding_len = max_seq_len_ - actual_len;
        // printf("actual_len 4: %d\n ", actual_len);
        // printf("remaining_frames 4: %d\n", remaining_frames);
        // printf("padding_len 4: %d\n", padding_len);

        if (remaining_frames > 0 && padding_len > 0) {
            int frames_per_padding = remaining_frames / padding_len;
            int remainder = remaining_frames % padding_len;

            for (int i = actual_len; i < pred_dur.size(); i++)
                pred_dur[i] = frames_per_padding;

            if (remainder > 0) {
                for (int i = actual_len; i < actual_len + remainder; i++) 
                    pred_dur[i] += 1;
            }
        }
        
        // total_frames = pred_dur.sum()
        total_frames = std::accumulate(pred_dur.begin(), pred_dur.end(), 0);
        // printf("total_frames: %d\n", total_frames);
    }

    std::vector<float> create_alignment_matrix_(const std::vector<int>& pred_dur, int total_frames) {
        // """创建对齐矩阵"""
        // indices = np.repeat(np.arange(self.max_seq_len_), pred_dur)
        // pred_aln_trg = np.zeros((self.max_seq_len_, total_frames), dtype=np.float32)
        // if len(indices) > 0:
        //     pred_aln_trg[indices, np.arange(total_frames)] = 1.0
        // return pred_aln_trg[np.newaxis, ...]

        std::vector<int> seq_range(max_seq_len_);
        std::iota(seq_range.begin(), seq_range.end(), 0);
        auto indices = np_repeat(seq_range, pred_dur);

        std::vector<float> pred_aln_trg(max_seq_len_ * total_frames);
        if (!indices.empty()) {
            int col = 0;
            for (auto i : indices) {
                pred_aln_trg[i * total_frames + col] = 1.0f;
                col++;
            }
        }

        return pred_aln_trg;
    }

    void compute_har_onnx_(std::vector<float>& F0_pred, std::vector<float>& har) {
        // Querying model inputs is possible but let's just assume one set for this translation or use a check.
        // For brevity, I'll use the older "tokens" set as default or try to match python logic if I can access names.
        int64_t input_shape[] = {F0_pred_shape_[0], F0_pred_shape_[1]};
        std::vector<const char*> input_names = {"F0_pred"};

        std::vector<Ort::Value> input_tensors;
        
        // Create tensors
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
        
        input_tensors.push_back(Ort::Value::CreateTensor<float>(
            memory_info, F0_pred.data(), F0_pred.size(), input_shape, F0_pred_shape_.size()));

        // Check model output name usually
        // Or get it from session
        std::vector<const char*> output_names = {"har"};

        auto output_tensors = model4_.Run(
            Ort::RunOptions{nullptr},
            input_names.data(),
            input_tensors.data(),
            input_tensors.size(),
            output_names.data(),
            output_names.size()
        );

        auto& output_tensor = output_tensors.front();
                
        // 获取输出信息
        auto tensor_info = output_tensor.GetTensorTypeAndShapeInfo();
        size_t element_count = tensor_info.GetElementCount();
        auto output_shape = tensor_info.GetShape();
        
        float* output_data = output_tensor.GetTensorMutableData<float>();
        
        har.resize(element_count);
        std::memcpy(har.data(), output_data, element_count * sizeof(float));
    }

    void postprocess_x_to_audio_(std::vector<float>& x, std::vector<float>& audio) {
        // 将频谱转换为音频波形
        // spec_part = x[:, :self.N_FFT//2+1, :]
        // phase_part = x[:, self.N_FFT//2+1:, :]
        int half_n_fft = N_FFT / 2 + 1;
        int num_frames = x_shape_[2];
        std::vector<float> spec_part(half_n_fft * num_frames);
        std::vector<float> phase_part(half_n_fft * num_frames);
        std::vector<float> cos_part(half_n_fft * num_frames);
        spec_part.assign(x.begin(), x.begin() + half_n_fft * num_frames);
        phase_part.assign(x.begin() + half_n_fft * num_frames, x.end());
        
        // spec = np.exp(spec_part)
        // phase = np.sin(phase_part)
        
        // spec_torch = torch.from_numpy(spec).float()
        // phase_torch = torch.from_numpy(phase).float()
        // cos_part = torch.sqrt(1.0 - phase_torch.pow(2).clamp(0, 1))
        
        // real = spec_torch * cos_part
        // imag = spec_torch * phase_torch
        // complex_spec = torch.complex(real, imag)

        for (int i = 0; i < half_n_fft * num_frames; i++) {
            spec_part[i] = expf(spec_part[i]);
            phase_part[i] = sinf(phase_part[i]);
            cos_part[i] = sqrtf(1.f - std::max(0.f, std::min(powf(phase_part[i], 2), 1.0f)));
        }

        FFT_RESULT complex_spec(half_n_fft, vector<complex<float>>(num_frames));
        for (int i = 0; i < half_n_fft; i++) {
            for (int n = 0; n < num_frames; n++) {
                float spec = spec_part[i * num_frames + n];

                float real_part = spec * cos_part[i * num_frames + n];
                float imag_part = spec * phase_part[i * num_frames + n];

                complex_spec[i][n] = std::complex<float>(real_part, imag_part);
            }
        }

        // audio = torch.istft(
        //     complex_spec, n_fft=self.N_FFT, hop_length=self.HOP_LENGTH,
        //     win_length=self.N_FFT, window=torch.hann_window(self.N_FFT),
        //     center=True, return_complex=False
        // )
        audio = librosa::Feature::istft(complex_spec, N_FFT, HOP_LENGTH, "hann", true, "reflect", false);
    }

private:
    TTSFrontend frontend_;

    int max_seq_len_;
    std::map<std::string, int> vocab_;
    std::string voice_path_;
    std::string voice_name_;
    std::vector<float> voice_tensor_;

    AxModelRunner model1_, model2_, model3_;
    Ort::Env env_;
    Ort::Session model4_{nullptr};
    Ort::AllocatorWithDefaultOptions allocator_;
    std::vector<float> duration_, d_, F0_pred_, N_pred_, asr_, x_;
    std::vector<int> duration_shape_, d_shape_, F0_pred_shape_, x_shape_;
};


Kokoro::Kokoro():
    impl_(std::make_unique<Kokoro::Impl>()) {

}

Kokoro::~Kokoro() {
    uninit();
}

bool Kokoro::init(AX_TTS_TYPE_E tts_type, AX_TTS_INIT_CONFIG* init_config) {
    return impl_->init(tts_type, init_config);
}

void Kokoro::uninit(void) {
    impl_.reset();
}

bool Kokoro::run(const std::string& text, AX_TTS_RUN_CONFIG* config, AX_TTS_AUDIO** audio) {
    return impl_->run(text, config, audio);
}