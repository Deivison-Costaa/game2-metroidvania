#!/usr/bin/env python3
"""
Generate a Hald-layout PNG LUT for cinematic color grading.

Output: assets/textures/lut_cinematic.png  (N²×N pixels, RGB8)
Hald layout: pixel(x=b*N+r, y=g) = graded(r/(N-1), g/(N-1), b/(N-1))

Color pipeline (Ori / Hollow Knight style):
  1. Lift-gamma-gain (teal shadows, warm highlights)
  2. Saturation ×1.10
  3. Subtle S-curve
"""

import pathlib, numpy as np
from PIL import Image

N = 64  # LUT resolution; change to 32 for a smaller file (256KB)

ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT  = ROOT / "assets" / "textures" / "lut_cinematic.png"

# ── Build identity grid [N, N, N, 3] float32 in [0, 1] ──────────────────────
r = np.linspace(0, 1, N, dtype=np.float32)
b_idx, g_idx, r_idx = np.meshgrid(
    np.arange(N), np.arange(N), np.arange(N), indexing="ij")  # [B,G,R]
vol = np.stack([
    r[r_idx],
    r[g_idx],
    r[b_idx],
], axis=-1)  # shape (N, N, N, 3) — vol[b][g][r] = (r_val, g_val, b_val)

# ── Cinematic pipeline ───────────────────────────────────────────────────────

def lift_gamma_gain(c):
    """Teal shadows, warm highlights."""
    lift  = np.array([-0.02,  0.02,  0.06], dtype=np.float32)
    gain  = np.array([ 0.05,  0.02, -0.04], dtype=np.float32)
    c = np.clip(c + lift * (1.0 - c), 0, 1)
    c = np.clip(c + gain * c,          0, 1)
    return c

def saturate(c, factor=1.10):
    """Boost saturation via Rec.709 luma."""
    luma = (0.2126 * c[..., 0] + 0.7152 * c[..., 1] + 0.0722 * c[..., 2])[..., None]
    return np.clip(luma + factor * (c - luma), 0, 1)

def scurve(c, toe=0.04, shoulder=0.92):
    """Soft S-curve: lift toe, pull shoulder."""
    c = np.where(c < 0.5,
        c * (1.0 - toe)      + toe      * c * c * 2.0,
        c * (1.0 - shoulder) + shoulder * (1.0 - (1.0 - c) * (1.0 - c) * 2.0))
    return np.clip(c, 0, 1)

vol = lift_gamma_gain(vol)
vol = saturate(vol)
vol = scurve(vol)

# ── Pack into Hald PNG (N²×N) ────────────────────────────────────────────────
# Pixel (x = b*N + r, y = g) = vol[b][g][r]
img = np.zeros((N, N * N, 3), dtype=np.uint8)
for b in range(N):
    for r_i in range(N):
        x = b * N + r_i
        # vol[b, :, r_i] is a column of G values for this (b, r) pair
        img[:, x, :] = (vol[b, :, r_i] * 255.0).astype(np.uint8)

OUT.parent.mkdir(parents=True, exist_ok=True)
Image.fromarray(img, "RGB").save(OUT)
print(f"✓ LUT saved → {OUT}  ({N}³ entries, {OUT.stat().st_size // 1024} KB)")
