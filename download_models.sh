#!/bin/bash
set -x

AX650_PATH="models-ax650"
AX630C_PATH="models-ax630c"
KOKORO_PATH="kokoro"

mkdir -p $AX650_PATH/$KOKORO_PATH
mkdir -p $AX630C_PATH/$KOKORO_PATH

export HF_ENDPOINT=https://hf-mirror.com

# Download 5 model files for AX650 (from inoryQwQ/kokoro.best)
hf download inoryQwQ/kokoro.best models/kokoro_enc_axera.axmodel --local-dir $AX650_PATH/$KOKORO_PATH
hf download inoryQwQ/kokoro.best models/kokoro_f0n.axmodel --local-dir $AX650_PATH/$KOKORO_PATH
hf download inoryQwQ/kokoro.best models/kokoro_dec.axmodel --local-dir $AX650_PATH/$KOKORO_PATH
hf download inoryQwQ/kokoro.best models/kokoro_har_noup.onnx --local-dir $AX650_PATH/$KOKORO_PATH
hf download inoryQwQ/kokoro.best models/kokoro_istft.onnx --local-dir $AX650_PATH/$KOKORO_PATH

# Download 5 model files for AX630C
hf download inoryQwQ/kokoro.best models/kokoro_enc_axera.axmodel --local-dir $AX630C_PATH/$KOKORO_PATH
hf download inoryQwQ/kokoro.best models/kokoro_f0n.axmodel --local-dir $AX630C_PATH/$KOKORO_PATH
hf download inoryQwQ/kokoro.best models/kokoro_dec.axmodel --local-dir $AX630C_PATH/$KOKORO_PATH
hf download inoryQwQ/kokoro.best models/kokoro_har_noup.onnx --local-dir $AX630C_PATH/$KOKORO_PATH
hf download inoryQwQ/kokoro.best models/kokoro_istft.onnx --local-dir $AX630C_PATH/$KOKORO_PATH

# Copy vocab.txt
cp dict/vocab.txt $AX650_PATH/$KOKORO_PATH
cp dict/vocab.txt $AX630C_PATH/$KOKORO_PATH

# Clone voices from kokoro.best repo
git clone https://hf-mirror.com/inoryQwQ/kokoro.best --depth=1 tmp_kokoro_best
cp -r tmp_kokoro_best/models/voices $AX650_PATH/$KOKORO_PATH
cp -r tmp_kokoro_best/models/voices $AX630C_PATH/$KOKORO_PATH
rm -rf tmp_kokoro_best

echo "ALL DONE"
