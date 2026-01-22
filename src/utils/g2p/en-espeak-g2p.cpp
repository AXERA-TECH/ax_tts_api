/**************************************************************************************************
 *
 * Copyright (c) 2019-2026 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include "utils/g2p/en-espeak-g2p.hpp"
#include "espeak-ng/speak_lib.h"

namespace utils {

EnEspeakG2P::EnEspeakG2P() {
    espeak_VOICE voice;
    memset(&voice, 0, sizeof(voice));
    
    voice.name = "English_(America)";
    voice.languages = "en-us";
    voice.gender = 2;           // 女性（1=男，2=女），设为0则不指定
    
    // voice.gender = 2;
    espeak_SetVoiceByProperties(&voice);
}

} // namespace utils