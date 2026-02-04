/**************************************************************************************************
 *
 * Copyright (c) 2019-2026 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#pragma once

#include <map>
#include <string>

#include "httplib.h"
#include "api/ax_tts_api.h"
#include "openai_err.hpp"

#define DEFAULT_PORT    8080
#define ASR_ENDPOINT    "/v1/audio/speech"

/* OpenAI-Compatible server
   Following API docs from: https://platform.openai.com/docs/api-reference/audio/createSpeech
   post http://IP:PORT/v1/audio/speech

 * Client usage:
 * - cURL:
 *      curl https://api.openai.com/v1/audio/speech \
        -H "Authorization: Bearer $OPENAI_API_KEY" \
        -H "Content-Type: application/json" \
        -d '{
            "model": "gpt-4o-mini-tts",
            "input": "The quick brown fox jumped over the lazy dog.",
            "voice": "alloy"
        }' \
        --output speech.mp3

 *
 * - Python:
 *  from pathlib import Path
    import openai

    speech_file_path = Path(__file__).parent / "speech.mp3"
    with openai.audio.speech.with_streaming_response.create(
        model="gpt-4o-mini-tts",
        voice="alloy",
        input="The quick brown fox jumped over the lazy dog."
        ) as response:
        response.stream_to_file(speech_file_path)


 * - JavaScript:
    import fs from "fs";
    import path from "path";
    import OpenAI from "openai";

    const openai = new OpenAI();

    const speechFile = path.resolve("./speech.mp3");

    async function main() {
    const mp3 = await openai.audio.speech.create({
        model: "gpt-4o-mini-tts",
        voice: "alloy",
        input: "Today is a wonderful day to build something people love!",
    });
    console.log(speechFile);
    const buffer = Buffer.from(await mp3.arrayBuffer());
    await fs.promises.writeFile(speechFile, buffer);
    }
    main();


 * Response:
 * Error: check https://platform.openai.com/docs/guides/error-codes
 *  {
        "error": {
            "message": "You exceeded your current quota, please check your plan and billing details.",
            "type": "insufficient_quota",
            "param": null,
            "code": 402
        }
    }

* Success:
    {
        "file": stream of pcm
    }
*/
class TTSServer {
public:
    TTSServer() = default;
    ~TTSServer() = default;

    bool init(const std::string& model_path);
    void start(int port = DEFAULT_PORT);
    void stop();

private:
    void setup_routes_();
    AX_TTS_HANDLE load_tts_(const std::string& model_name);
    // 设置CORS头
    void set_CORS_headers_(httplib::Response& res);
    
    /*
     * request: Content-Type: multipart/form-data
     * {
     *      "input": "The text to generate audio for."
     *      "language": Supports zh, en, ja, etc.
     *      "model": Supports kokoro and melotts
     *      "speed": The speed of the generated audio. Select a value from 0.25 to 4.0. 1.0 is the default.
     * }
    */
    bool check_request_(const httplib::Request& req, httplib::Response& res);

private:
    std::map<std::string, AX_TTS_HANDLE> handles_;
    httplib::Server srv_;
    std::string model_path_;
};