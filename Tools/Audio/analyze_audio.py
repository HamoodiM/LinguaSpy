"""Loudness / harshness report for WAV files (plain Python + numpy; no audio libraries needed).

    python Tools/Audio/analyze_audio.py Saved/AudioAnalysis [filter]

WAVs come from the editor: import the source (.ogg/.wav) as a SoundWave, then export it with SoundExporterWAV
(see Tools/Content/audio_import_cc0.py). Reported per file:
  dur      duration (s)
  peak     peak level (dBFS)
  rms      RMS level (dBFS)
  centroid spectral centroid (Hz) - higher = brighter / more piercing
  hf%      share of energy above 4 kHz - high values read as harsh
  flat     spectral flatness 0..1 - near 0 = pure tones (the Phase 3 "ringing"), near 1 = noise
  f0 a->b  dominant frequency in the first vs last third (rising / falling contour)
"""
import os
import sys
import wave

import numpy as np


def load(path):
    with wave.open(path) as w:
        n, ch, width, rate = w.getnframes(), w.getnchannels(), w.getsampwidth(), w.getframerate()
        raw = w.readframes(n)
    dtype = {1: np.uint8, 2: np.int16, 4: np.int32}[width]
    x = np.frombuffer(raw, dtype=dtype).astype(np.float64)
    if width == 1:
        x = (x - 128.0) / 128.0
    else:
        x /= float(2 ** (8 * width - 1))
    if ch > 1:
        x = x.reshape(-1, ch).mean(axis=1)
    return x, rate


def db(v):
    return 20.0 * np.log10(max(v, 1e-9))


def dominant(x, rate):
    if len(x) < 256:
        return 0.0
    spec = np.abs(np.fft.rfft(x * np.hanning(len(x))))
    freqs = np.fft.rfftfreq(len(x), 1.0 / rate)
    spec[freqs < 60] = 0
    return float(freqs[int(np.argmax(spec))])


def analyze(path):
    x, rate = load(path)
    if len(x) == 0:
        return None
    spec = np.abs(np.fft.rfft(x * np.hanning(len(x)))) ** 2
    freqs = np.fft.rfftfreq(len(x), 1.0 / rate)
    total = spec.sum() + 1e-18
    centroid = float((freqs * spec).sum() / total)
    hf = float(spec[freqs > 4000].sum() / total)
    band = spec[(freqs > 60) & (freqs < 12000)] + 1e-18
    flatness = float(np.exp(np.mean(np.log(band))) / np.mean(band))
    third = max(len(x) // 3, 1)
    return {
        "dur": len(x) / rate,
        "peak": db(np.max(np.abs(x))),
        "rms": db(np.sqrt(np.mean(x ** 2))),
        "centroid": centroid,
        "hf": hf * 100.0,
        "flat": flatness,
        "f0a": dominant(x[:third], rate),
        "f0b": dominant(x[-third:], rate),
    }


def main():
    folder = sys.argv[1] if len(sys.argv) > 1 else "Saved/AudioAnalysis"
    filt = sys.argv[2].lower() if len(sys.argv) > 2 else ""
    print(f"{'file':44} {'dur':>6} {'peak':>6} {'rms':>6} {'centroid':>8} {'hf%':>5} {'flat':>5}  f0 a->b")
    for name in sorted(os.listdir(folder)):
        if not name.lower().endswith(".wav") or filt not in name.lower():
            continue
        r = analyze(os.path.join(folder, name))
        if r:
            print(f"{name[:44]:44} {r['dur']:6.2f} {r['peak']:6.1f} {r['rms']:6.1f} {r['centroid']:8.0f} {r['hf']:5.1f} {r['flat']:5.2f}  "
                  f"{r['f0a']:.0f}->{r['f0b']:.0f}")


if __name__ == "__main__":
    main()
