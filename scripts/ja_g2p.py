import argparse

def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--text", "-t", type=str, required=True)
    return parser.parse_args()

def tokens_to_ps(tokens):
    return ''.join(t.phonemes + (' ' if t.whitespace else '') for t in tokens).strip()

def main():
    args = get_args()
    try:
        from misaki import ja
    except Exception as e:
        raise SystemExit(f"Failed to import misaki.ja. Install with `pip install misaki[ja]`. Error: {e}")

    g2p = ja.JAG2P()
    tokens = g2p(args.text)
    phonemes = tokens_to_ps(tokens)
    print(phonemes)

if __name__ == "__main__":
    main()
