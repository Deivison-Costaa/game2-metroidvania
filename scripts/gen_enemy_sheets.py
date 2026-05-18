#!/usr/bin/env python3
"""
Generate enemy sprite sheets for M4:
  assets/sprites/enemy_walker.png  — 128×128  red slime-knight (4×4 cells of 32×32)
  assets/sprites/enemy_flyer.png   — 128×128  purple bat (4×4 cells of 32×32)
  assets/sprites/enemy_ranged.png  — 128×128  green goblin archer (4×4 cells of 32×32)

Sheet layout (same for all):
  Row 0: Patrol  (4 frames — walking cycle)
  Row 1: Chase   (4 frames — faster walk / charge)
  Row 2: Attack  (4 frames — windup/swing)
  Row 3: Hurt (2 frames) + Dead (2 frames)
"""
from pathlib import Path
from typing import Tuple
from PIL import Image, ImageDraw
import math

CELL   = 32
COLS   = 4
ROWS   = 4
TRANS  : tuple = (0, 0, 0, 0)
OUT    = Path(__file__).parent.parent / "assets" / "sprites"
OUT.mkdir(parents=True, exist_ok=True)

RGBA = Tuple[int, int, int, int]


# ── Walker (slime-knight) — red/orange ────────────────────────────────────────

W_BODY  : RGBA = (200,  60,  40, 255)  # deep red
W_LIGHT : RGBA = (230,  90,  60, 255)
W_DARK  : RGBA = (130,  25,  15, 255)
W_EYE   : RGBA = (255, 230,  50, 255)


def draw_walker(d: ImageDraw.ImageDraw, ox: int, oy: int,
                leg: int = 0, attack: bool = False,
                hurt: bool = False, dead: bool = False) -> None:
    color = W_LIGHT if hurt else W_BODY
    if dead:
        # Lie flat
        d.ellipse([ox+4, oy+20, ox+28, oy+30], fill=W_DARK)
        d.ellipse([ox+8, oy+18, ox+24, oy+28], fill=color)
        return
    # Body — rounded blob
    d.ellipse([ox+6, oy+6, ox+26, oy+24], fill=color)
    d.ellipse([ox+8, oy+4, ox+24, oy+22], fill=color)
    # Spiky top (3 spikes)
    for sx in [9, 16, 23]:
        d.polygon([(ox+sx-2, oy+6), (ox+sx, oy+1), (ox+sx+2, oy+6)], fill=W_DARK)
    # Eyes
    if not hurt:
        d.ellipse([ox+11, oy+9, ox+14, oy+12], fill=W_EYE)
        d.ellipse([ox+18, oy+9, ox+21, oy+12], fill=W_EYE)
        d.point([(ox+12, oy+10), (ox+19, oy+10)], fill=(0,0,0,255))
    # Legs
    offsets = [(-3, 0), (3, 0)] if leg == 0 else [(-4, 1), (4, 1)]
    for lox, loy in offsets:
        d.rectangle([ox+13+lox, oy+22+loy, ox+15+lox, oy+28], fill=W_DARK)
    # Attack arm
    if attack:
        d.line([(ox+22, oy+12), (ox+30, oy+8)], fill=W_DARK, width=3)


def make_walker() -> None:
    img = Image.new("RGBA", (CELL*COLS, CELL*ROWS), TRANS)
    d   = ImageDraw.Draw(img)

    for col in range(4):  # Row 0: patrol
        draw_walker(d, col*CELL, 0*CELL, leg=col%2)
    for col in range(4):  # Row 1: chase (body tilted forward)
        draw_walker(d, col*CELL, 1*CELL, leg=col%2)
    for col in range(4):  # Row 2: attack
        draw_walker(d, col*CELL, 2*CELL, leg=0, attack=(col in [1,2]))
    for col in range(2):  # Row 3: hurt
        draw_walker(d, col*CELL, 3*CELL, leg=0, hurt=True)
    for col in range(2,4):  # Row 3: dead
        draw_walker(d, col*CELL, 3*CELL, dead=True)

    out = OUT / "enemy_walker.png"
    img.save(out, "PNG")
    print(f"Generated: {out}")


# ── Flyer (bat) — purple/violet ───────────────────────────────────────────────

F_BODY  : RGBA = (130,  50, 180, 255)
F_WING  : RGBA = ( 90,  30, 130, 255)
F_LIGHT : RGBA = (170,  80, 220, 255)
F_EYE   : RGBA = (255,  60,  60, 255)


def draw_bat(d: ImageDraw.ImageDraw, ox: int, oy: int,
             wing: int = 0, attack: bool = False,
             hurt: bool = False, dead: bool = False) -> None:
    if dead:
        d.ellipse([ox+10, oy+16, ox+22, oy+28], fill=F_WING)
        return

    # Wings
    wh = [6, 2, 10, 4][wing % 4]  # wing height varies by frame
    d.polygon([
        (ox+16, oy+14),
        (ox+2,  oy+14-wh),
        (ox+4,  oy+20)
    ], fill=F_WING)
    d.polygon([
        (ox+16, oy+14),
        (ox+30, oy+14-wh),
        (ox+28, oy+20)
    ], fill=F_WING)

    # Body
    d.ellipse([ox+10, oy+10, ox+22, oy+24], fill=F_BODY if not hurt else F_LIGHT)
    # Ears
    d.polygon([(ox+12, oy+10), (ox+10, oy+5), (ox+14, oy+10)], fill=F_BODY)
    d.polygon([(ox+20, oy+10), (ox+22, oy+5), (ox+18, oy+10)], fill=F_BODY)
    # Eyes
    d.ellipse([ox+12, oy+13, ox+15, oy+16], fill=F_EYE)
    d.ellipse([ox+17, oy+13, ox+20, oy+16], fill=F_EYE)
    # Attack fangs
    if attack:
        d.polygon([(ox+14, oy+22), (ox+13, oy+27), (ox+16, oy+22)], fill=(240,240,240,255))
        d.polygon([(ox+18, oy+22), (ox+19, oy+27), (ox+16, oy+22)], fill=(240,240,240,255))


def make_flyer() -> None:
    img = Image.new("RGBA", (CELL*COLS, CELL*ROWS), TRANS)
    d   = ImageDraw.Draw(img)

    for col in range(4):
        draw_bat(d, col*CELL, 0*CELL, wing=col)
    for col in range(4):
        draw_bat(d, col*CELL, 1*CELL, wing=(col+1)%4)
    for col in range(4):
        draw_bat(d, col*CELL, 2*CELL, wing=col%2, attack=(col in [1,2]))
    for col in range(2):
        draw_bat(d, col*CELL, 3*CELL, wing=0, hurt=True)
    for col in range(2,4):
        draw_bat(d, col*CELL, 3*CELL, dead=True)

    out = OUT / "enemy_flyer.png"
    img.save(out, "PNG")
    print(f"Generated: {out}")


# ── Ranged (goblin archer) — dark green ──────────────────────────────────────

R_SKIN  : RGBA = ( 60, 130,  50, 255)
R_CLOTH : RGBA = ( 35,  80,  30, 255)
R_LIGHT : RGBA = ( 90, 170,  70, 255)
R_BOW   : RGBA = (140, 100,  40, 255)
R_EYE   : RGBA = (255, 200,  50, 255)


def draw_goblin(d: ImageDraw.ImageDraw, ox: int, oy: int,
                bow_raised: bool = False, firing: bool = False,
                hurt: bool = False, dead: bool = False) -> None:
    if dead:
        d.rectangle([ox+4, oy+22, ox+28, oy+30], fill=R_CLOTH)
        d.ellipse([ox+4, oy+18, ox+16, oy+28], fill=R_SKIN)
        return

    skin = R_LIGHT if hurt else R_SKIN

    # Head
    d.ellipse([ox+11, oy+4, ox+21, oy+14], fill=skin)
    # Pointy ears
    d.polygon([(ox+11, oy+7), (ox+7, oy+5), (ox+11, oy+10)], fill=skin)
    d.polygon([(ox+21, oy+7), (ox+25, oy+5), (ox+21, oy+10)], fill=skin)
    # Eyes
    d.point([(ox+14, oy+8), (ox+18, oy+8)], fill=R_EYE)

    # Body
    d.rectangle([ox+12, oy+14, ox+20, oy+24], fill=R_CLOTH)

    # Legs
    d.rectangle([ox+12, oy+24, ox+15, oy+30], fill=R_CLOTH)
    d.rectangle([ox+17, oy+24, ox+20, oy+30], fill=R_CLOTH)

    # Bow arm
    bow_y = oy + (6 if bow_raised else 10)
    d.arc([ox+21, bow_y, ox+29, bow_y+14], start=270, end=90, fill=R_BOW, width=2)
    if firing:
        d.line([(ox+21, bow_y+7), (ox+3, bow_y+7)], fill=(230,200,100,255), width=1)


def make_ranged() -> None:
    img = Image.new("RGBA", (CELL*COLS, CELL*ROWS), TRANS)
    d   = ImageDraw.Draw(img)

    for col in range(4):  # Row 0: idle
        draw_goblin(d, col*CELL, 0*CELL, bow_raised=(col%2==0))
    for col in range(4):  # Row 1: attack (aiming)
        draw_goblin(d, col*CELL, 1*CELL, bow_raised=True, firing=(col >= 2))
    for col in range(4):  # Row 2: idle repeat (no chase anim for ranged)
        draw_goblin(d, col*CELL, 2*CELL, bow_raised=(col%2==0))
    for col in range(2):  # Row 3: hurt
        draw_goblin(d, col*CELL, 3*CELL, hurt=True)
    for col in range(2,4):  # Row 3: dead
        draw_goblin(d, col*CELL, 3*CELL, dead=True)

    out = OUT / "enemy_ranged.png"
    img.save(out, "PNG")
    print(f"Generated: {out}")


if __name__ == "__main__":
    make_walker()
    make_flyer()
    make_ranged()
    print("Done.")
