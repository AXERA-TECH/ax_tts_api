# Installing espeak varies across platforms, this silent install works on Colab:
#!apt-get -qq -y install espeak-ng > /dev/null 2>&1

#!pip install -q "misaki[en]" phonemizer-fork

from misaki import en, espeak

fallback = espeak.EspeakFallback(british=False) # en-us

g2p = en.G2P(trf=False, british=False, fallback=fallback) # no transformer, American English

text = 'Hello, World!'

phonemes, tokens = g2p(text)

print(phonemes) # həlˈO, wˈɜɹld!

from misaki.token import MToken

for token in tokens:
    ps = fallback(token)
    print(token.text, ps)