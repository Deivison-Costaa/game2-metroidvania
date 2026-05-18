#!/usr/bin/env python3
"""Generate a procedural ambient loop WAV for game2 (stdlib only)."""
import math
import struct
import wave
import os

SAMPLE_RATE = 44100
LOOP_DURATION = 8.0   # seconds — seamless loop (fade in/out matched)
OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "audio", "music")
os.makedirs(OUT_DIR, exist_ok=True)

def write_wav_stereo(filename: str, left: list[float], right: list[float]) -> None:
    path = os.path.join(OUT_DIR, filename)
    n = min(len(left), len(right))
    peak = max(max(abs(s) for s in left + right), 1e-9)
    interleaved = []
    for i in range(n):
        interleaved.append(int(max(-32767, min(32767, left[i]  / peak * 28000))))
        interleaved.append(int(max(-32767, min(32767, right[i] / peak * 28000))))
    with wave.open(path, "w") as f:
        f.setnchannels(2)
        f.setsampwidth(2)
        f.setframerate(SAMPLE_RATE)
        f.writeframes(struct.pack(f"<{len(interleaved)}h", *interleaved))
    print(f"  wrote {path}")

def gen_ambient_loop():
    n = int(SAMPLE_RATE * LOOP_DURATION)
    left  = [0.0] * n
    right = [0.0] * n

    # 4-voice drone pad — low-frequency oscillators with slow vibrato
    voices = [
        # (freq Hz, amp, vibrato_rate, vibrato_depth, pan)
        (55.00, 0.30, 0.12, 0.003, -0.6),   # A1 — bass root, panned left
        (82.41, 0.22, 0.17, 0.004,  0.5),   # E2 — fifth, right
        (110.0, 0.18, 0.08, 0.002,  0.1),   # A2 — octave, center
        (164.8, 0.12, 0.21, 0.005, -0.2),   # E3 — high fifth
    ]

    # Slow filter sweep (amplitude modulation to mimic filter open/close)
    sweep_rate = 1.0 / LOOP_DURATION    # one sweep per loop

    for i in range(n):
        t = i / SAMPLE_RATE
        sweep_env = 0.6 + 0.4 * math.sin(2 * math.pi * sweep_rate * t)

        mix = 0.0
        for (freq, amp, vib_rate, vib_depth, pan) in voices:
            vibrato = 1.0 + vib_depth * math.sin(2 * math.pi * vib_rate * t)
            sample  = amp * math.sin(2 * math.pi * freq * vibrato * t) * sweep_env

            # Slight odd-harmonic saturation
            sample += amp * 0.08 * math.sin(2 * math.pi * freq * 3.0 * vibrato * t)

            pan_l = (1.0 - pan) * 0.5
            pan_r = (1.0 + pan) * 0.5
            left[i]  += sample * pan_l
            right[i] += sample * pan_r

        # Soft reverb via delay taps (all-pass approximation)
        if i >= int(SAMPLE_RATE * 0.07):
            left[i]  += 0.18 * left[i  - int(SAMPLE_RATE * 0.07)]
        if i >= int(SAMPLE_RATE * 0.11):
            right[i] += 0.15 * right[i - int(SAMPLE_RATE * 0.11)]

    # Cross-fade loop ends to ensure seamless looping (10% fade)
    fade_samples = int(n * 0.10)
    for i in range(fade_samples):
        alpha = i / fade_samples
        left[i]       = left[i]       * alpha + left[n - fade_samples + i]  * (1 - alpha)
        right[i]      = right[i]      * alpha + right[n - fade_samples + i] * (1 - alpha)
        left[n - fade_samples + i]  *= alpha
        right[n - fade_samples + i] *= alpha

    write_wav_stereo("ambient_loop.wav", left, right)

if __name__ == "__main__":
    print("Generating ambient music loop...")
    gen_ambient_loop()
    print("Done.")
