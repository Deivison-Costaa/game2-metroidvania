#!/usr/bin/env python3
"""Generate procedural SFX WAV files for game2 (no dependencies beyond stdlib)."""
import math
import struct
import wave
import os

SAMPLE_RATE = 44100
OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "audio", "sfx")
os.makedirs(OUT_DIR, exist_ok=True)

def write_wav(filename: str, samples: list[float]) -> None:
    """Write normalised float samples as 16-bit mono WAV."""
    path = os.path.join(OUT_DIR, filename)
    peak = max(abs(s) for s in samples) or 1.0
    data = [int(max(-32767, min(32767, s / peak * 32767))) for s in samples]
    with wave.open(path, "w") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(SAMPLE_RATE)
        f.writeframes(struct.pack(f"<{len(data)}h", *data))
    print(f"  wrote {path}")

def adsr(t: float, attack: float, decay: float, sustain: float, release_start: float, release_end: float) -> float:
    if t < attack:
        return t / attack
    if t < attack + decay:
        return 1.0 - (t - attack) / decay * (1.0 - sustain)
    if t < release_start:
        return sustain
    if t < release_end:
        return sustain * (1.0 - (t - release_start) / (release_end - release_start))
    return 0.0

def noise() -> float:
    import random
    return random.uniform(-1.0, 1.0)

# ── attack.wav ────────────────────────────────────────────────────────────────
# Short metallic whoosh: high-frequency sine sweep + noise burst
def gen_attack():
    dur = 0.12
    n = int(SAMPLE_RATE * dur)
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        frac = t / dur
        freq = 800.0 + 2400.0 * frac          # sweep 800 → 3200 Hz
        sine = math.sin(2 * math.pi * freq * t)
        env = adsr(t, 0.005, 0.02, 0.3, 0.08, 0.12)
        samples.append((0.6 * sine + 0.4 * noise()) * env)
    write_wav("attack.wav", samples)

# ── hit.wav ───────────────────────────────────────────────────────────────────
# Low thud: sub-sine at 60 Hz + noise impulse
def gen_hit():
    dur = 0.18
    n = int(SAMPLE_RATE * dur)
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        freq = 80.0 * math.exp(-t * 12.0)    # decaying pitch
        sine = math.sin(2 * math.pi * freq * t)
        env = adsr(t, 0.002, 0.05, 0.1, 0.12, 0.18)
        samples.append((0.5 * sine + 0.5 * noise()) * env)
    write_wav("hit.wav", samples)

# ── jump.wav ──────────────────────────────────────────────────────────────────
# Ascending chirp: frequency sweeps upward quickly
def gen_jump():
    dur = 0.10
    n = int(SAMPLE_RATE * dur)
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        frac = t / dur
        freq = 200.0 + 800.0 * frac
        sine = math.sin(2 * math.pi * freq * t)
        env = adsr(t, 0.005, 0.03, 0.4, 0.07, 0.10)
        samples.append(sine * env)
    write_wav("jump.wav", samples)

# ── land.wav ──────────────────────────────────────────────────────────────────
# Heavy thud: low sine + wide noise
def gen_land():
    dur = 0.14
    n = int(SAMPLE_RATE * dur)
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        freq = 55.0 * math.exp(-t * 8.0)
        sine = math.sin(2 * math.pi * freq * t)
        env = adsr(t, 0.001, 0.04, 0.0, 0.06, 0.14)
        samples.append((0.4 * sine + 0.6 * noise()) * env)
    write_wav("land.wav", samples)

# ── death.wav ─────────────────────────────────────────────────────────────────
# Descending multi-sine glide + noise tail
def gen_death():
    dur = 0.55
    n = int(SAMPLE_RATE * dur)
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        frac = t / dur
        freq1 = 440.0 * math.exp(-frac * 2.5)
        freq2 = 660.0 * math.exp(-frac * 2.5)
        s = (0.5 * math.sin(2 * math.pi * freq1 * t)
             + 0.3 * math.sin(2 * math.pi * freq2 * t)
             + 0.2 * noise())
        env = adsr(t, 0.01, 0.1, 0.5, 0.35, 0.55)
        samples.append(s * env)
    write_wav("death.wav", samples)

# ── pickup.wav ────────────────────────────────────────────────────────────────
# Ascending arpeggio: three short sine tones
def gen_pickup():
    dur = 0.30
    n = int(SAMPLE_RATE * dur)
    tones = [(0.00, 0.10, 523.25),   # C5
             (0.10, 0.20, 659.25),   # E5
             (0.20, 0.30, 783.99)]   # G5
    samples = [0.0] * n
    for i in range(n):
        t = i / SAMPLE_RATE
        for (t0, t1, freq) in tones:
            if t0 <= t < t1:
                lt = t - t0
                ld = t1 - t0
                env = adsr(lt, 0.01, 0.02, 0.7, ld * 0.6, ld)
                samples[i] += math.sin(2 * math.pi * freq * t) * env
    write_wav("pickup.wav", samples)

if __name__ == "__main__":
    print("Generating SFX...")
    gen_attack()
    gen_hit()
    gen_jump()
    gen_land()
    gen_death()
    gen_pickup()
    print("Done.")
