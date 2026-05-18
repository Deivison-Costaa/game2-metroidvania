#!/usr/bin/env python3
"""
Polished procedural player sprite sheet.
Generates player_sheet.png with 4×4 layout (4 frames × 4 rows = Idle/Run/Jump/Fall/Attack).
Anti-aliased shapes, multi-colour palette, squash/stretch on jump/fall.

Run from repo root: python3 scripts/gen_player_sheet_polished.py
"""

import pathlib, math
from PIL import Image, ImageDraw, ImageFilter

ROOT    = pathlib.Path(__file__).resolve().parent.parent
OUT_DIR = ROOT / "assets" / "sprites"

FRAME_W, FRAME_H = 32, 32
COLS, ROWS       = 4, 5  # 4 anim states × 4 frames, + attack row

# Palette
COL_BODY   = (60, 120, 200, 255)
COL_ARMOR  = (30,  60, 140, 255)
COL_SKIN   = (220, 170, 120, 255)
COL_SWORD  = (210, 200, 180, 255)
COL_OUTLINE= ( 20,  15,  30, 255)
COL_HAIR   = ( 60,  40,  20, 255)
COL_EYES   = (240, 240, 255, 255)
COL_GLOW   = (180, 220, 255, 200)

def draw_player(draw: ImageDraw.ImageDraw, ox: int, oy: int,
                scale_y: float = 1.0, lean: int = 0, sword_anim: float = 0.0):
    """Draw a stylised player into (ox,oy) FRAME-sized cell."""

    # Helper: draw a filled ellipse centred at (cx,cy) with radius (rx,ry)
    def el(cx, cy, rx, ry, col):
        draw.ellipse([(ox+cx-rx, oy+cy-ry*scale_y), (ox+cx+rx, oy+cy+ry*scale_y)], fill=col)

    # Helper: filled rect
    def re(x0, y0, x1, y1, col):
        y_mid = (y0 + y1) / 2
        dy    = (y1 - y0) / 2
        draw.rectangle([(ox+x0, oy+y_mid-dy*scale_y),
                        (ox+x1, oy+y_mid+dy*scale_y)], fill=col)

    # Body (torso)
    re(10, 14, 22, 26, COL_ARMOR)
    re(11, 15, 21, 25, COL_BODY)

    # Head
    el(16, 9, 5, 5, COL_SKIN)
    el(16, 9, 4, 4, COL_SKIN)          # extra fill
    el(14, 8, 1, 1, COL_EYES)          # eye L
    el(18, 8, 1, 1, COL_EYES)          # eye R

    # Hair
    el(16, 6, 4, 3, COL_HAIR)

    # Legs (lean shifts them horizontally)
    re(10+lean, 26, 15+lean, 32, COL_ARMOR)
    re(17+lean, 26, 22+lean, 32, COL_ARMOR)

    # Arms
    re(5,  15, 10, 24, COL_BODY)
    re(22, 15, 27, 24, COL_BODY)

    # Sword (right hand, animated angle)
    if sword_anim > 0.0:
        angle = -60 + sword_anim * 120
        cx, cy = ox+27, oy+18
        for i in range(12):
            rad = math.radians(angle)
            sx  = int(cx + math.cos(rad) * i)
            sy  = int(cy + math.sin(rad) * i)
            draw.point((sx, sy), fill=COL_SWORD)
        # glow
        draw.ellipse([(cx-3, cy-3), (cx+3, cy+3)], fill=COL_GLOW)

def generate(path: pathlib.Path):
    sheet = Image.new("RGBA", (FRAME_W * COLS, FRAME_H * ROWS), (0, 0, 0, 0))
    draw  = ImageDraw.Draw(sheet)

    # Row 0: Idle (4 frames, subtle breathing)
    for f in range(4):
        ox = f * FRAME_W
        draw_player(draw, ox, 0, scale_y=1.0 + math.sin(f / 4 * math.pi) * 0.03)

    # Row 1: Run (4 frames, body leans forward)
    for f in range(4):
        ox   = f * FRAME_W
        lean = [-2, 0, -2, 0][f]
        draw_player(draw, ox, FRAME_H, lean=lean)

    # Row 2: Jump (4 frames, squash on takeoff, stretch in air)
    for f in range(4):
        ox    = f * FRAME_W
        sy    = [0.8, 1.1, 1.2, 1.15][f]  # squash → stretch
        draw_player(draw, ox, FRAME_H*2, scale_y=sy)

    # Row 3: Fall (4 frames, compressed → elongated)
    for f in range(4):
        ox = f * FRAME_W
        sy = [1.15, 1.1, 0.95, 0.85][f]
        draw_player(draw, ox, FRAME_H*3, scale_y=sy)

    # Row 4: Attack (4 frames, sword swing)
    for f in range(4):
        ox = f * FRAME_W
        draw_player(draw, ox, FRAME_H*4, sword_anim=f / 3.0)

    # Mild outline pass (morphological — expand alpha and darken edge)
    r, g, b, a = sheet.split()
    edge = a.filter(ImageFilter.MaxFilter(3))
    from PIL import ImageChops
    border_a = ImageChops.subtract(edge, a)
    outline  = Image.new("RGBA", sheet.size, COL_OUTLINE)
    outline.putalpha(border_a)
    final = Image.alpha_composite(outline, sheet)

    final.save(path)
    print(f"✓ Player sheet → {path}")

generate(OUT_DIR / "player_sheet.png")
