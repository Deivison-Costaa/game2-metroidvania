#!/usr/bin/env python3
"""Generate a placeholder 256×512 boss sprite sheet for game2.

Layout: 4 columns × 8 rows, each cell 64×64 px.
Row mapping:
  0 — Stance Idle (4 frames)
  1 — Stance Slash (4 frames)
  2 — Enrage Slash (4 frames)
  3 — Desperate Radial (4 frames)
  4 — Hurt         (4 frames)
  5 — Dead         (4 frames)
  6-7 — (reserved/transition)
"""
from PIL import Image, ImageDraw, ImageFont
import os

CELL = 64
COLS = 4
ROWS = 8
OUT = os.path.join(os.path.dirname(__file__), "..", "assets", "sprites", "boss.png")

def make_boss_sheet():
    img = Image.new("RGBA", (COLS * CELL, ROWS * CELL), (0, 0, 0, 0))
    d   = ImageDraw.Draw(img)

    row_colors = [
        (80, 60, 120),    # 0 Stance Idle — purple-grey
        (140, 60, 60),    # 1 Stance Slash — dark red
        (200, 100, 30),   # 2 Enrage Slash — orange
        (200, 50, 50),    # 3 Desperate Radial — red
        (180, 180, 180),  # 4 Hurt — white-grey
        (60, 60, 60),     # 5 Dead — dark grey
        (100, 80, 160),   # 6 Transition — purple
        (100, 80, 160),   # 7 Transition
    ]

    for row in range(ROWS):
        base_col = row_colors[row]
        for col in range(COLS):
            ox, oy = col * CELL, row * CELL
            # Body (taller and wider than regular enemies)
            pulse = 1.0 - col * 0.06  # slight darkening per frame = animation feel
            c = tuple(int(v * pulse) for v in base_col)
            # Torso
            d.rectangle([ox+8,  oy+12, ox+56, oy+52], fill=c + (255,))
            # Head (slightly lighter)
            hc = tuple(min(255, v + 40) for v in c)
            d.ellipse([ox+16, oy+2, ox+48, oy+24], fill=hc + (255,))
            # Eyes (white + dark)
            d.ellipse([ox+21, oy+7, ox+29, oy+15], fill=(240,240,240,255))
            d.ellipse([ox+35, oy+7, ox+43, oy+15], fill=(240,240,240,255))
            d.ellipse([ox+23, oy+9, ox+27, oy+13], fill=(30,20,40,255))
            d.ellipse([ox+37, oy+9, ox+41, oy+13], fill=(30,20,40,255))
            # Attack stance arm (for attack rows)
            if row in (1, 2, 3):
                arm_col = tuple(min(255, v + 60) for v in c)
                arm_x = ox + 56 + col * 2  # extends further per frame
                d.rectangle([ox+52, oy+16, min(ox+63, arm_x+6), oy+28],
                            fill=arm_col + (255,))
            # Health bar decoration (top)
            d.rectangle([ox+4, oy+58, ox+60, oy+62], fill=(60,60,60,200))
            d.rectangle([ox+4, oy+58, ox+4+(56*(COLS-col)//COLS), oy+62],
                       fill=(180,40,40,220))

    img.save(OUT)
    print(f"  wrote {OUT} ({COLS*CELL}×{ROWS*CELL})")

if __name__ == "__main__":
    print("Generating boss sprite sheet...")
    make_boss_sheet()
    print("Done.")
