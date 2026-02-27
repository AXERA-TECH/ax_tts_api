#!/usr/bin/env bash
set -euo pipefail

ip="${1:-10.126.33.238}"
port="${2:-8080}"
out_dir="${3:-../ja_voices_630c}"

voices=(jf_gongitsune jf_nezumi jf_tebukuro jm_kumo jf_alpha)
texts=(
"おはようございます。きょうはよいてんきですね。"
"あしたのかいぎはごぜんじゅうじからはじまります。"
"おんせいのひんしつをかくにんしています。よろしくおねがいします。"
)

mkdir -p "$out_dir"

for v in "${voices[@]}"; do
  s=1
  for t in "${texts[@]}"; do
    python3 "test_tts_server.py" \
      --ip "$ip" \
      --port "$port" \
      -l ja \
      -t "$t" \
      --voice "$v" \
      --output "${out_dir}/${v}_s${s}"
    s=$((s+1))
  done
done

ls -l "$out_dir"
