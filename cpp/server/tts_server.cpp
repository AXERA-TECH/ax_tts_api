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
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <fstream>
#include <limits.h>
#include <cstdlib>
#include <cstring>
 
#include "tts_server.hpp"
#include "utils/logger.h"
#include "openai_err.hpp"
#include "utils/nlohmann/json.hpp"
#include "utils/AudioFile.h"

static std::string pick_path(const char* env_var, const char* default_rel) {
    const char* env = std::getenv(env_var);
    if (env && env[0] != '\0') {
        return std::string(env);
    }
    return std::string(default_rel);
}

static bool is_jp_kana_only(const std::string& s) {
    auto is_kana_or_allowed = [](uint32_t cp) {
        // whitespace
        if (cp == 0x20 || cp == 0x09 || cp == 0x0A || cp == 0x0D) return true;
        // ASCII digits and punctuation (reject letters)
        if (cp >= 0x30 && cp <= 0x39) return true;
        if ((cp >= 0x21 && cp <= 0x2F) || (cp >= 0x3A && cp <= 0x40) ||
            (cp >= 0x5B && cp <= 0x60) || (cp >= 0x7B && cp <= 0x7E)) return true;

        // Hiragana, Katakana, extensions, halfwidth katakana
        if ((cp >= 0x3040 && cp <= 0x309F) || (cp >= 0x30A0 && cp <= 0x30FF) ||
            (cp >= 0x31F0 && cp <= 0x31FF) || (cp >= 0xFF65 && cp <= 0xFF9F)) return true;

        // Common Japanese punctuation
        switch (cp) {
            case 0x3001: // 、 
            case 0x3002: // 。
            case 0x300C: // 「
            case 0x300D: // 」
            case 0x300E: // 『
            case 0x300F: // 』
            case 0x3010: // 【
            case 0x3011: // 】
            case 0x3014: // 〔
            case 0x3015: // 〕
            case 0x301C: // 〜
            case 0x2026: // …
            case 0x30FB: // ・
            case 0x30FC: // ー
                return true;
            default:
                return false;
        }
    };

    const unsigned char* p = reinterpret_cast<const unsigned char*>(s.data());
    size_t i = 0;
    while (i < s.size()) {
        uint32_t cp = 0;
        size_t adv = 1;
        unsigned char c = p[i];
        if (c < 0x80) {
            cp = c;
            adv = 1;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < s.size()) {
            cp = ((c & 0x1F) << 6) | (p[i + 1] & 0x3F);
            adv = 2;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < s.size()) {
            cp = ((c & 0x0F) << 12) | ((p[i + 1] & 0x3F) << 6) | (p[i + 2] & 0x3F);
            adv = 3;
        } else if ((c & 0xF8) == 0xF0 && i + 3 < s.size()) {
            cp = ((c & 0x07) << 18) | ((p[i + 1] & 0x3F) << 12) |
                 ((p[i + 2] & 0x3F) << 6) | (p[i + 3] & 0x3F);
            adv = 4;
        } else {
            return false;
        }

        if (!is_kana_or_allowed(cp)) return false;
        i += adv;
    }
    return true;
}

static std::map<std::string, AX_TTS_TYPE_E> MODEL_MAP = {
        {"kokoro", AX_KOKORO},
        {"melotts", AX_MELOTTS}
    };

class InlineTaskQueue final : public httplib::TaskQueue {
public:
    bool enqueue(std::function<void()> fn) override {
        fn();
        return true;
    }
    void shutdown() override {}
};

int get_interface_ip(const char *interface_name, char *ip_address_buffer) {
    int fd;
    struct ifreq ifr;

    // Ensure input buffers are valid
    if (interface_name == NULL || ip_address_buffer == NULL) {
        return -1;
    }

    // Create a socket
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket error");
        return -1;
    }

    // Specify the interface name
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0'; // Ensure null termination

    // Get the IP address
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
        perror("ioctl error");
        close(fd);
        return -1;
    }

    // Convert the binary IP address to a human-readable string
    struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
    strcpy(ip_address_buffer, inet_ntoa(addr->sin_addr));

    // Close the socket
    close(fd);

    return 0;
}

// Read a binary file; returns empty vector on failure.
static std::vector<char> read_binary_file(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        ALOGE("Cannot open file: %s", filepath.c_str());
        return {};
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        ALOGE("Error reading file: %s", filepath.c_str());
        return {};
    }

    return buffer;
}

TTSServer::~TTSServer() {
    stop();
    cleanup_handles_();
}

bool TTSServer::init(const std::string& model_path,
                     const std::string& espeak_data_path,
                     const std::string& jieba_dict_path) {
    model_path_ = model_path;
    espeak_data_path_ = espeak_data_path;
    jieba_dict_path_ = jieba_dict_path;

    // Run handlers in the server thread to avoid thread-unsafe TTS runtime issues.
    // httplib takes ownership via unique_ptr and deletes the queue after each request.
    this->srv_.new_task_queue = [] { return new InlineTaskQueue(); };

    this->setup_routes_();
    // Preload default model in main thread to avoid thread-unsafe init in request handler
    if (!this->load_tts_("kokoro")) {
        ALOGE("Preload kokoro failed!");
        return false;
    }
    ALOGI("TTSServer init success");
    return true;
}

void TTSServer::start(int port) {
    char ip_buffer[INET_ADDRSTRLEN]; // INET_ADDRSTRLEN is max length for IPv4 addr string
    const char* interface = "eth0";

    if (get_interface_ip(interface, ip_buffer) == 0) {
        ALOGI("Starting server at %s:%d", ip_buffer, port);
    } else {
        ALOGE("Failed to get IP address for %s", interface);
        return;
    }

    this->srv_.listen("0.0.0.0", port);
}

void TTSServer::stop() {
    ALOGI("Terminate server.");
    this->srv_.stop();
}

void TTSServer::cleanup_handles_() {
    for (auto& kv : handles_) {
        AX_TTS_Uninit(kv.second);
    }
    handles_.clear();
}

// ================ PRIVATE ================
void TTSServer::setup_routes_() {
    this->srv_.Post(TTS_ENDPOINT, [this](const httplib::Request& req, httplib::Response& res) {
        // 1. 设置CORS头
        set_CORS_headers_(res);

        // 2. 检查参数
        nlohmann::json json_data;
        if (!this->check_request_(req, res, json_data)) {
            ALOGE("Check request param failed!");
            return;
        }

        // 3. 获取参数
        std::string input_text;
        if (json_data.contains("phonemes")) {
            input_text = "__PHONEMES__:" + std::string(json_data["phonemes"]);
        } else {
            input_text = json_data["input"];
        }
        std::string model = json_data["model"];
        std::string language = json_data["instructions"];
        std::string voice;
        if (json_data.contains("voice")) {
            voice = json_data["voice"];
        }
        float speed = 1.0f;
        if (json_data.contains("speed"))
            speed = json_data["speed"];

        if (language == "ja") {
            static const std::string kPhonemePrefix = "__PHONEMES__:";
            if (input_text.rfind(kPhonemePrefix, 0) != 0 && !is_jp_kana_only(input_text)) {
                ErrorResponse openai_res(OPENAI_ERR_BAD_REQUEST,
                    "Japanese input must be kana-only (hiragana/katakana) or provide phonemes.",
                    "input");
                openai_res.to_res(res);
                return;
            }
        }

        // 4. 加载tts模型, 不会重复加载
        auto handle = this->load_tts_(model);

        // 5. 运行模型
        AX_TTS_RUN_CONFIG run_config;
        run_config.fade_out = 0.3f;
        run_config.speed = speed;
        if (model == "kokoro")
            run_config.sample_rate = 24000;
        else
            run_config.sample_rate = 44100;

        std::string voice_str = voice;
        if (voice_str.empty()) {
            if (language == "ja") voice_str = "jf_gongitsune";
            else if (language == "zh") voice_str = "zf_xiaoxiao";
            else voice_str = "af_heart";
        }
        run_config.language = language.c_str();
        run_config.voice = voice_str.c_str();

        AX_TTS_AUDIO* audio = NULL;
        AX_TTS_ERROR_E err = AX_TTS_Run(handle, input_text.c_str(), &run_config, &audio);
        if (err != AX_TTS_OK) {
            ALOGE("AX_TTS_Run failed! err=%d", err);
            free(audio);
            ErrorResponse openai_res(OPENAI_ERR_INTERNAL_SERVER_ERROR, "AX_TTS_Run failed!", "");
            openai_res.to_res(res);
            return;
        }

        // Template must be a character array, not a string constant, last 6 chars must be XXXXXX
        char tmp_filename[] = "/tmp/tts_server_outputXXXXXX"; 
        int fd;

        // Create the unique file and get the file descriptor
        fd = mkstemp(tmp_filename);

        if (fd == -1) {
            ALOGE("mkstemp failed");
            ErrorResponse openai_res(OPENAI_ERR_INTERNAL_SERVER_ERROR, "mkstemp failed!", "");
            openai_res.to_res(res);
            return;
        }

        // template now holds the unique filename (e.g., "/tmp/mytempfilea1B2c3")
        ALOGD("Created temporary file: %s", tmp_filename);

        // You can now write to the file using the file descriptor, 
        // for example, with write() or by using fdopen() to get a FILE* stream
        // ... use fd ... 

        // Close the file descriptor
        close(fd);


        AudioFile<float> audio_file;
        std::vector<std::vector<float> > audio_samples{std::vector<float>(audio->data, audio->data + audio->num_samples)};
        audio_file.setAudioBuffer(audio_samples);
        audio_file.setSampleRate(run_config.sample_rate);
        if (!audio_file.save(tmp_filename)) {
            ALOGE("Save audio file failed!\n");
            ErrorResponse openai_res(OPENAI_ERR_INTERNAL_SERVER_ERROR, "Save audio file failed!", "");
            openai_res.to_res(res);
            free(audio);
            return;
        }

        ALOGI("Saved tts result to %s, samplerate=%d, num_samples=%d", tmp_filename, run_config.sample_rate, audio->num_samples);

        free(audio);

        std::vector<char> buffer = read_binary_file(tmp_filename);

        // Remove the file after use
        if (unlink(tmp_filename) != 0) {
            perror("unlink failed");
            ErrorResponse openai_res(OPENAI_ERR_INTERNAL_SERVER_ERROR, "unlink failed!", "");
            openai_res.to_res(res);
            return;
        }

        // Set the content with the correct MIME type for WAV files
        res.set_content(buffer.data(), buffer.size(), "audio/wav"); //
        res.status = 200;

        return;
    });
}

AX_TTS_HANDLE TTSServer::load_tts_(const std::string& model_name) {
    if (this->handles_.find(model_name) != this->handles_.end()) {
        return this->handles_.at(model_name);
    } else {
        // try to new one
        if (MODEL_MAP.find(model_name) == MODEL_MAP.end()) {
            ALOGE("Cannot find model of %s", model_name.c_str());
            return nullptr;
        }

        AX_TTS_TYPE_E tts_type = MODEL_MAP.at(model_name);
        ALOGI("Initializing %s ...", model_name.c_str());

        AX_TTS_INIT_CONFIG init_config;
        std::string espeak_path;
        std::string jieba_path;
        if (AX_KOKORO == tts_type) {
            init_config.max_seq_len = 96;
            espeak_path = espeak_data_path_.empty()
                ? pick_path("AX_TTS_ESPEAK_DATA_PATH", "espeak-ng-data")
                : espeak_data_path_;
            jieba_path = jieba_dict_path_.empty()
                ? pick_path("AX_TTS_JIEBA_DICT_PATH", "dict")
                : jieba_dict_path_;
            init_config.espeak_data_path = espeak_path.c_str();
            init_config.jieba_dict_path = jieba_path.c_str();
        } else if (AX_MELOTTS == tts_type) {
            init_config.max_seq_len = 128;
        }
        init_config.model_path = model_path_.c_str();
        
        AX_TTS_HANDLE new_handle = NULL;
        AX_TTS_ERROR_E err = AX_TTS_Init(tts_type, &init_config, &new_handle);
        if (err != AX_TTS_OK) {
            ALOGE("AX_TTS_Init failed! err=%d", err);
            return nullptr;
        }

        this->handles_.insert({model_name, new_handle});
        return new_handle;
    }
}

void TTSServer::set_CORS_headers_(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", 
                    "Content-Type, X-Array-Name, X-Array-Description, X-Array-Size");
}

bool TTSServer::check_request_(const httplib::Request& req, httplib::Response& res, nlohmann::json& json_data) {
    // 1. 检查Content-Type
    if (!req.has_header("Content-Type") ||
        req.get_header_value("Content-Type").find("application/json") == std::string::npos) {
            ALOGE("Content-Type must be application/json. Current is %s", req.get_header_value("Content-Type").c_str());
            ErrorResponse openai_res(OPENAI_ERR_BAD_REQUEST, "Content-Type must be application/json.", "Content-Type");
            openai_res.to_res(res);
            return false;
    }

    try {
        json_data = nlohmann::json::parse(req.body);

        // 2. 检查model
        {
            if (!json_data.contains("model")) {
                ALOGE("\"model\" field must be provided.");
                ErrorResponse openai_res(OPENAI_ERR_BAD_REQUEST, "\"model\" field must be provided.", "model");
                openai_res.to_res(res);
                return false;
            }

            std::string model = json_data["model"];
            if (MODEL_MAP.find(model) == MODEL_MAP.end()) {
                ALOGE("%s not found in server.", model.c_str());
                ErrorResponse openai_res(OPENAI_ERR_NOT_FOUND, model + "not found in server.", "model");
                openai_res.to_res(res);
                return false;
            }

            // 获取模型
            auto handle = this->load_tts_(model);
            if (!handle) {
                ALOGE("Load asr failed!");
                ErrorResponse openai_res(OPENAI_ERR_NOT_FOUND, "Load tts failed.", "model");
                openai_res.to_res(res);
                return false;
            }
        }
        
        // 3. 检查language
        {
            if (!json_data.contains("instructions")) {
                ErrorResponse openai_res(OPENAI_ERR_BAD_REQUEST, "\"instructions\" field must be provided.", "instructions");
                openai_res.to_res(res);
                return false;
            }
        }

        // 4. 检查input
        {
            bool has_input = json_data.contains("input");
            bool has_phonemes = json_data.contains("phonemes");
            if (!has_input && !has_phonemes) {
                ErrorResponse openai_res(OPENAI_ERR_BAD_REQUEST, "\"input\" or \"phonemes\" field must be provided.", "input");
                openai_res.to_res(res);
                return false;
            }
        }
    } catch (const nlohmann::json::parse_error& e) {
        // Handle JSON parsing errors
        ErrorResponse openai_res(OPENAI_ERR_BAD_REQUEST, std::string("Error parsing JSON: ") + std::string(e.what()), "");
        openai_res.to_res(res);
        return false;
    }

    return true;
}
