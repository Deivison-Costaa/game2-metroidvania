#!/usr/bin/env python3
"""
Generate procedural sprite assets for M3:
  assets/sprites/player_sheet.png  — 128x128 knight sprite sheet (4x4 grid of 32x32 frames)
  assets/sprites/dummy.png         — 32x32 training dummy

Layout (row x col, each cell 32x32):
  Row 0: Idle (frames 0-3 — 2 unique + 2 mirrored)
  Row 1: Run  (frames 0-3 — 4 unique)
  Row 2: Jump/Fall (0=jump windup, 1=apex, 2=fall peak, 3=fall)
  Row 3: Attack (0=windup, 1=swing, 2=hit, 3=recovery)
"""
from pathlib import Path
from PIL import Image, ImageDraw

CELL = 32
COLS = 4
ROWS = 4
SHEET_W = CELL * COLS  # 128
SHEET_H = CELL * ROWS  # 128

# Palette
BLUE   = (74,  111, 165, 255)   # armor body
GRAY   = (154, 165, 180, 255)   # lighter armor / visor
DARK   = (40,  55,  80,  255)   # outline / shadow
WHITE  = (232, 232, 232, 255)   # sword/highlight
RED    = (180, 60,  60,  255)   # sword edge on attack
TRANS  = (0, 0, 0, 0)

OUT_DIR = Path(__file__).parent.parent / "assets" / "sprites"
OUT_DIR.mkdir(parents=True, exist_ok=True)


def draw_knight(draw: ImageDraw.ImageDraw, ox: int, oy: int,
                body_dy: int = 0, leg_phase: int = 0,
                sword_angle: str = "none",
                crouch: bool = False) -> None:
    """Draw a simple pixel-art knight at (ox, oy) offset inside a 32x32 cell."""
    # Head (5x5 at top-center)
    hx, hy = ox + 13, oy + 2 + body_dy
    draw.rectangle([hx, hy, hx+5, hy+5], fill=GRAY)      # visor
    draw.rectangle([hx+1, hy+1, hx+4, hy+4], fill=BLUE)  # face
    draw.point([(hx+2, hy+3), (hx+3, hy+3)], fill=DARK)  # eyes

    # Body (7x8)
    bx, by = ox + 12, oy + 8 + body_dy
    bh = 7 if not crouch else 5
    draw.rectangle([bx, by, bx+8, by+bh], fill=BLUE)
    draw.rectangle([bx+1, by+1, bx+7, by+bh-1], fill=GRAY)  # chest highlight

    # Legs
    leg_y = oy + 16 + body_dy
    if not crouch:
        if leg_phase == 0:   # neutral
            draw.rectangle([ox+12, leg_y, ox+15, leg_y+7], fill=DARK)
            draw.rectangle([ox+17, leg_y, ox+20, leg_y+7], fill=DARK)
        elif leg_phase == 1:  # step A
            draw.rectangle([ox+11, leg_y, ox+14, leg_y+6], fill=DARK)
            draw.rectangle([ox+11, leg_y+5, ox+15, leg_y+7], fill=DARK)  # foot A
            draw.rectangle([ox+18, leg_y, ox+21, leg_y+8], fill=DARK)
        elif leg_phase == 2:  # step B
            draw.rectangle([ox+10, leg_y, ox+13, leg_y+8], fill=DARK)
            draw.rectangle([ox+18, leg_y, ox+21, leg_y+6], fill=DARK)
            draw.rectangle([ox+17, leg_y+5, ox+22, leg_y+7], fill=DARK)  # foot B
        elif leg_phase == 3:  # mid-air tuck
            draw.rectangle([ox+11, leg_y-2, ox+14, leg_y+4], fill=DARK)
            draw.rectangle([ox+17, leg_y-2, ox+20, leg_y+4], fill=DARK)
    else:  # crouched
        draw.rectangle([ox+12, leg_y-2, ox+19, leg_y+2], fill=DARK)

    # Sword
    if sword_angle == "up":       # windup
        draw.line([(ox+22, oy+6+body_dy), (ox+28, oy+2+body_dy)], fill=WHITE, width=2)
    elif sword_angle == "mid":    # swing
        draw.line([(ox+22, oy+10+body_dy), (ox+30, oy+10+body_dy)], fill=WHITE, width=2)
        draw.line([(ox+29, oy+9+body_dy), (ox+31, oy+11+body_dy)], fill=RED, width=1)
    elif sword_angle == "down":   # follow-through
        draw.line([(ox+22, oy+12+body_dy), (ox+28, oy+18+body_dy)], fill=WHITE, width=2)
    elif sword_angle == "idle":
        draw.line([(ox+21, oy+9+body_dy), (ox+21, oy+16+body_dy)], fill=GRAY, width=1)


def make_sheet() -> None:
    sheet = Image.new("RGBA", (SHEET_W, SHEET_H), TRANS)
    d     = ImageDraw.Draw(sheet)

    # Row 0: Idle (subtle body bob — even frames bob down 1px)
    for col in range(COLS):
        ox, oy = col * CELL, 0
        bob = 1 if col % 2 == 1 else 0
        draw_knight(d, ox, oy, body_dy=bob, leg_phase=0, sword_angle="idle")

    # Row 1: Run
    for col in range(COLS):
        ox, oy = col * CELL, CELL
        draw_knight(d, ox, oy, body_dy=col % 2, leg_phase=(col % 2) + 1, sword_angle="idle")

    # Row 2: Jump (0-1) and Fall (2-3)
    # 0 = windup (body down, legs bent)
    draw_knight(d, 0, CELL*2, body_dy=2,  leg_phase=3, crouch=True)
    # 1 = apex (body up, legs extended)
    draw_knight(d, CELL, CELL*2, body_dy=-2, leg_phase=3)
    # 2 = fall peak (body neutral, legs tuck start)
    draw_knight(d, CELL*2, CELL*2, body_dy=0, leg_phase=3)
    # 3 = fall land (body down)
    draw_knight(d, CELL*3, CELL*2, body_dy=3, leg_phase=3, crouch=True)

    # Row 3: Attack
    # 0 = windup
    draw_knight(d, 0,      CELL*3, body_dy=0, sword_angle="up")
    # 1 = swing (active hitbox frame)
    draw_knight(d, CELL,   CELL*3, body_dy=0, sword_angle="mid")
    # 2 = hit contact
    draw_knight(d, CELL*2, CELL*3, body_dy=0, sword_angle="mid")
    # 3 = recovery
    draw_knight(d, CELL*3, CELL*3, body_dy=1, sword_angle="down")

    out = OUT_DIR / "player_sheet.png"
    sheet.save(out, "PNG")
    print(f"Generated: {out}  ({SHEET_W}x{SHEET_H})")


def make_dummy() -> None:
    img = Image.new("RGBA", (CELL, CELL), TRANS)
    d   = ImageDraw.Draw(img)

    # Simple stick-figure post (training dummy)
    POST   = (100, 80, 60, 255)   # wooden post
    HEAD   = (200, 180, 140, 255) # head
    STRAW  = (180, 160, 100, 255) # body

    # Pole
    d.rectangle([15, 0, 17, 31], fill=POST)
    # Head
    d.ellipse([10, 3, 22, 15], fill=HEAD, outline=POST)
    # Torso
    d.rectangle([11, 15, 21, 26], fill=STRAW, outline=POST)
    # Base
    d.rectangle([10, 27, 22, 31], fill=POST)

    out = OUT_DIR / "dummy.png"
    img.save(out, "PNG")
    print(f"Generated: {out}  (32x32)")


if __name__ == "__main__":
    make_sheet()
    make_dummy()
    print("Done — run cmake --build to pick up the new assets.")
