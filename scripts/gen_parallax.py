#!/usr/bin/env python3
"""
Generate 3 parallax background layers for M4:
  assets/sprites/parallax_sky.png       — 1024×256  gradient sky
  assets/sprites/parallax_mountains.png — 1024×256  mountain silhouettes
  assets/sprites/parallax_trees.png     — 1024×256  tree silhouettes
"""
from pathlib import Path
from typing import Tuple
from PIL import Image, ImageDraw
import math

W, H   = 1024, 256
OUT    = Path(__file__).parent.parent / "assets" / "sprites"
OUT.mkdir(parents=True, exist_ok=True)

RGBA = Tuple[int, int, int, int]


# ── Sky ────────────────────────────────────────────────────────────────────────

def make_sky() -> None:
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d   = ImageDraw.Draw(img)

    # Vertical gradient: deep night-blue at top → purple-dusk at bottom
    for y in range(H):
        t   = y / (H - 1)
        r   = int(15  + t * 50)
        g   = int(10  + t * 20)
        b   = int(55  + t * 80)
        a   = 255
        d.line([(0, y), (W - 1, y)], fill=(r, g, b, a))

    # Stars — deterministic pseudo-random
    import random
    rng = random.Random(42)
    for _ in range(120):
        sx = rng.randint(0, W - 1)
        sy = rng.randint(0, int(H * 0.55))
        brightness = rng.randint(180, 255)
        sz = rng.choice([1, 1, 1, 2])
        d.ellipse([sx, sy, sx+sz, sy+sz], fill=(brightness, brightness, brightness, 255))

    out = OUT / "parallax_sky.png"
    img.save(out, "PNG")
    print(f"Generated: {out}  ({W}x{H})")


# ── Mountains ─────────────────────────────────────────────────────────────────

def make_mountains() -> None:
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d   = ImageDraw.Draw(img)

    # Far mountains (lighter gray-blue)
    far_color  : RGBA = (55, 60, 90, 220)
    near_color : RGBA = (38, 40, 65, 240)

    def mountain_profile(peaks: list[tuple[int,int]], base_y: int) -> list[tuple[int,int]]:
        pts: list[tuple[int,int]] = [(0, base_y)]
        for px, py in peaks:
            pts.append((px, py))
        pts.append((W, base_y))
        return pts

    # Far range
    far_peaks = [(50, 180), (130, 130), (220, 155), (310, 110), (390, 140),
                 (480, 100), (560, 130), (650, 90),  (740, 115), (830, 80),
                 (910, 110), (980, 130), (1024, 150)]
    d.polygon(mountain_profile(far_peaks, H), fill=far_color)

    # Near range
    near_peaks = [(0, 210), (80, 170), (180, 145), (270, 165), (360, 135),
                  (450, 150), (540, 120), (620, 145), (700, 110), (790, 135),
                  (870, 115), (950, 140), (1024, 160)]
    d.polygon(mountain_profile(near_peaks, H), fill=near_color)

    # Snow caps on tallest peaks
    snow: RGBA = (220, 230, 240, 200)
    for px, py in [(310, 110), (480, 100), (650, 90), (830, 80)]:
        d.ellipse([px-12, py-4, px+12, py+20], fill=snow)

    out = OUT / "parallax_mountains.png"
    img.save(out, "PNG")
    print(f"Generated: {out}  ({W}x{H})")


# ── Trees ──────────────────────────────────────────────────────────────────────

def make_trees() -> None:
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d   = ImageDraw.Draw(img)

    # Ground strip
    ground: RGBA = (20, 30, 20, 255)
    d.rectangle([0, H - 30, W, H], fill=ground)

    # Tree silhouettes
    dark_green  : RGBA = (12, 40, 20, 255)
    mid_green   : RGBA = (18, 58, 28, 255)
    trunk_color : RGBA = (20, 15, 10, 255)

    import random
    rng = random.Random(7)

    x = 0
    while x < W + 40:
        tree_h  = rng.randint(100, 200)
        trunk_w = rng.randint(6, 12)
        trunk_h = rng.randint(20, 40)
        crown_w = rng.randint(30, 70)

        # Trunk
        tx = x - trunk_w // 2
        d.rectangle([tx, H - 30 - trunk_h, tx + trunk_w, H - 30], fill=trunk_color)

        # Crown (3 stacked triangles for pine-like shape)
        for layer in range(3):
            top_y   = H - 30 - trunk_h - tree_h + layer * (tree_h // 4)
            base_y  = top_y + tree_h // 3 + 10
            half_w  = crown_w - layer * 8
            color   = mid_green if layer == 1 else dark_green
            d.polygon([(x, top_y), (x - half_w, base_y), (x + half_w, base_y)],
                      fill=color)

        x += rng.randint(40, 90)

    out = OUT / "parallax_trees.png"
    img.save(out, "PNG")
    print(f"Generated: {out}  ({W}x{H})")


if __name__ == "__main__":
    make_sky()
    make_mountains()
    make_trees()
    print("Done.")
