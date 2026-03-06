#!/usr/bin/env bash
set -euo pipefail

ip="${1:-127.0.0.1}"
port="${2:-8080}"
out_dir="${3:-/tmp/tts_smoke}"

mkdir -p "$out_dir"

post_json() {
  local name="$1"
  local json="$2"
  local out="$3"
  local code="000"

  if command -v curl >/dev/null 2>&1; then
    code=$(curl -s -o "$out" -w '%{http_code}' \
      -H 'Content-Type: application/json' \
      -d "$json" \
      "http://${ip}:${port}/v1/audio/speech")
  elif command -v wget >/dev/null 2>&1; then
    wget -q -O "$out" --server-response \
      --header='Content-Type: application/json' \
      --post-data="$json" \
      "http://${ip}:${port}/v1/audio/speech" 2>&1 | awk '/^  HTTP/{code=$2} END{print code}' > /tmp/.tts_http_code
    if [[ -s /tmp/.tts_http_code ]]; then
      code=$(cat /tmp/.tts_http_code)
    fi
  else
    echo "No curl or wget found"
    return 1
  fi

  echo "$name http:$code size:$(stat -c%s "$out" 2>/dev/null || echo 0)"
  if [[ "$code" == "200" ]]; then
    [[ -s "$out" ]] || { echo "$name output empty"; return 1; }
  fi
}

echo "== TTS smoke test =="
echo "target: ${ip}:${port}"
echo "out: ${out_dir}"

# English
post_json "en_ok" \
  '{"model":"kokoro","input":"Hello world","voice":"af_heart","response_format":"wav","speed":1.0,"instructions":"en"}' \
  "${out_dir}/en.wav"

# Chinese (requires server started with --jieba_dict_path)
post_json "zh_ok" \
  '{"model":"kokoro","input":"早上好，今天天气不错。","voice":"zf_xiaobei","response_format":"wav","speed":1.0,"instructions":"zh"}' \
  "${out_dir}/zh.wav"

# Japanese kana (hiragana/katakana only)
post_json "ja_kana_ok" \
  '{"model":"kokoro","input":"おはようございます。きょうはよいてんきですね。","voice":"jf_gongitsune","response_format":"wav","speed":1.0,"instructions":"ja"}' \
  "${out_dir}/ja_kana.wav"

# Japanese kana (katakana)
post_json "ja_katakana_ok" \
  '{"model":"kokoro","input":"コンニチハ。ヨロシクオネガイシマス。","voice":"jf_gongitsune","response_format":"wav","speed":1.0,"instructions":"ja"}' \
  "${out_dir}/ja_katakana.wav"

# Long Japanese kana (chunking)
post_json "ja_kana_long_ok" \
  '{"model":"kokoro","input":"おはようございます。きょうはよいてんきですね。あしたのかいぎはごぜんじゅうじからはじまります。おんせいのひんしつをかくにんしています。","voice":"jf_gongitsune","response_format":"wav","speed":1.0,"instructions":"ja"}' \
  "${out_dir}/ja_kana_long.wav"

# Japanese with kanji should be rejected (400)
post_json "ja_kanji_reject" \
  '{"model":"kokoro","input":"おはようございます。今日は良い天気ですね。","voice":"jf_gongitsune","response_format":"wav","speed":1.0,"instructions":"ja"}' \
  "${out_dir}/ja_kanji.err"

echo "done"
