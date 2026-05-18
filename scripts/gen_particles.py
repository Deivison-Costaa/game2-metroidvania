#!/usr/bin/env python3
"""Generate particle_spark.png — 16x16 RGBA with Gaussian alpha gradient."""
import os, math
from PIL import Image

SIZE   = 16
cx = cy = SIZE / 2.0
sigma  = SIZE / 4.5  # controls softness

img  = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
data = []
for y in range(SIZE):
    for x in range(SIZE):
        dx = x + 0.5 - cx
        dy = y + 0.5 - cy
        d2 = (dx * dx + dy * dy) / (sigma * sigma)
        alpha = int(255 * math.exp(-d2))
        data.append((255, 255, 255, alpha))

img.putdata(data)

out_dir  = os.path.join(os.path.dirname(__file__), "..", "assets", "sprites")
out_path = os.path.join(out_dir, "particle_spark.png")
os.makedirs(out_dir, exist_ok=True)
img.save(out_path)
print(f"Saved {out_path}  ({SIZE}x{SIZE} RGBA Gaussian spark)")
