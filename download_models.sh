#!/bin/bash
set -x

AX650_PATH="models-ax650"
AX630C_PATH="models-ax630c"
KOKORO_PATH="kokoro"

mkdir -p $AX650_PATH
mkdir -p $AX630C_PATH

mkdir -p $AX650_PATH/$KOKORO_PATH

mkdir -p $AX630C_PATH/$KOKORO_PATH

export HF_ENDPOINT=https://hf-mirror.com

# kokoro
hf download AXERA-TECH/kokoro.axera models/kokoro_part1_96.axmodel --local-dir $AX650_PATH/$KOKORO_PATH
hf download AXERA-TECH/kokoro.axera models/kokoro_part2_96.axmodel --local-dir $AX650_PATH/$KOKORO_PATH
hf download AXERA-TECH/kokoro.axera models/kokoro_part3_96.axmodel --local-dir $AX650_PATH/$KOKORO_PATH
hf download AXERA-TECH/kokoro.axera models/model4_har_sim.onnx --local-dir $AX650_PATH/$KOKORO_PATH
hf download AXERA-TECH/kokoro.axera models_620E/kokoro_part1_96.axmodel --local-dir $AX630C_PATH/$KOKORO_PATH
hf download AXERA-TECH/kokoro.axera models_620E/kokoro_part2_96.axmodel --local-dir $AX630C_PATH/$KOKORO_PATH
hf download AXERA-TECH/kokoro.axera models_620E/kokoro_part3_96.axmodel --local-dir $AX630C_PATH/$KOKORO_PATH
hf download AXERA-TECH/kokoro.axera models_620E/model4_har_sim.onnx --local-dir $AX630C_PATH/$KOKORO_PATH
mv $AX650_PATH/$KOKORO_PATH/models/* $AX650_PATH/$KOKORO_PATH
mv $AX630C_PATH/$KOKORO_PATH/models_620E/* $AX630C_PATH/$KOKORO_PATH

cp dict/vocab.txt $AX650_PATH/$KOKORO_PATH
cp dict/vocab.txt $AX630C_PATH/$KOKORO_PATH
git clone https://github.com/AXERA-TECH/kokoro.axera.git --depth=1
cp -r kokoro.axera/cpp/voices $AX650_PATH/$KOKORO_PATH
cp -r kokoro.axera/cpp/voices $AX630C_PATH/$KOKORO_PATH
rm -rf kokoro.axera

echo "ALL DONE"