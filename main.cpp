#include <stdio.h>
#include "utils/cmdline.hpp"
#include "utils/timer.hpp"
#include "utils/AudioFile.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "ax_tts_api.h"
#ifdef __cplusplus
}
#endif

int main(int argc, char** argv) {
    cmdline::parser cmd;
    cmd.add<std::string>("wav", 'w', "wav file", true, "");
#if defined(CHIP_AX650)    
    cmd.add<std::string>("model_path", 'p', "model path which contains whisper/ sensevoice/", false, "./models-ax650");
#else
    cmd.add<std::string>("model_path", 'p', "model path which contains whisper/ sensevoice/", false, "./models-ax630c");
#endif
    cmd.add<std::string>("language", 'l', "en, zh", false, "zh");
    cmd.parse_check(argc, argv);

    // 0. get app args, can be removed from user's app
    auto wav_file = cmd.get<std::string>("wav");
    auto model_path = cmd.get<std::string>("model_path");
    auto language = cmd.get<std::string>("language");

    return 0;
}