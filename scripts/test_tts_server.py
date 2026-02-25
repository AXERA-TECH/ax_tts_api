import argparse
from openai import OpenAI

def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip", type=str, required=True)
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--text", "-t", type=str, required=True)
    parser.add_argument("--model", "-m", type=str, required=False, default="kokoro", choices=["kokoro",])
    parser.add_argument("--language", "-l", type=str, required=False, default="en", choices=["en", "zh"])
    parser.add_argument("--speed", type=float, default=1.0)
    parser.add_argument("--response_format", "-f", type=str, default="wav")
    parser.add_argument("--output", type=str, default="output")
    return parser.parse_args()

args = get_args()
client = OpenAI(
    base_url=f'http://{args.ip}:{args.port}/v1',
    api_key="dummy_key"
)

speech = client.audio.speech.create(
            input=args.text,
            model=args.model,
            voice="alloy",
            response_format=args.response_format,
            speed=args.speed,
            instructions=args.language
        )

audio_file = args.output + '.' + args.response_format
with open(audio_file, "wb") as f:
    f.write(speech.content)

print(f"Saved result to {audio_file}")