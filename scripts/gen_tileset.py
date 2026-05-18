#!/usr/bin/env python3
"""
Generate assets/tilesets/test_tileset.png — 4×2 grid of 32×32 tiles.

Tile GIDs (1-based, Tiled convention):
  GID 1 (local 0): stone ground — used for ground and solid walls
  GID 2 (local 1): stone ground lighter variant
  GID 3 (local 2): platform — wooden plank top
  GID 4 (local 3): platform underside — dark wood
  GID 5 (local 4): stone wall — darker block
  GID 6 (local 5): stone wall lighter variant
  GID 7 (local 6): dirt / filler tile
  GID 8 (local 7): grass / decoration
"""
from pathlib import Path
from typing import Tuple
from PIL import Image, ImageDraw

CELL   = 32
COLS   = 4
ROWS   = 2
OUT    = Path(__file__).parent.parent / "assets" / "tilesets"
OUT.mkdir(parents=True, exist_ok=True)

RGBA = Tuple[int, int, int, int]

STONE_DARK   : RGBA = ( 60,  60,  70, 255)
STONE_MID    : RGBA = ( 80,  80,  92, 255)
STONE_LIGHT  : RGBA = (105, 105, 118, 255)
STONE_HL     : RGBA = (130, 130, 145, 255)
PLANK_DARK   : RGBA = ( 90,  65,  35, 255)
PLANK_MID    : RGBA = (120,  90,  50, 255)
PLANK_LIGHT  : RGBA = (155, 120,  70, 255)
DIRT_DARK    : RGBA = ( 80,  55,  30, 255)
DIRT_MID     : RGBA = (110,  80,  45, 255)
GRASS_GREEN  : RGBA = ( 70, 150,  50, 255)
TRANS        : RGBA = (  0,   0,   0,  0)


def draw_stone_tile(d: ImageDraw.ImageDraw, ox: int, oy: int,
                    base: RGBA, mid: RGBA, hl: RGBA) -> None:
    """Stone block with beveled edge and speckle noise."""
    d.rectangle([ox, oy, ox+31, oy+31], fill=base)
    # Bevel highlight (top/left edges)
    d.line([(ox, oy), (ox+30, oy)],    fill=hl,  width=2)
    d.line([(ox, oy), (ox, oy+30)],    fill=hl,  width=2)
    # Bevel shadow (bottom/right edges)
    d.line([(ox, oy+31), (ox+31, oy+31)], fill=STONE_DARK, width=2)
    d.line([(ox+31, oy), (ox+31, oy+31)], fill=STONE_DARK, width=2)
    # Interior texture — simple cross pattern
    d.line([(ox+16, oy+4), (ox+16, oy+27)], fill=mid, width=1)
    d.line([(ox+4, oy+16), (ox+27, oy+16)], fill=mid, width=1)
    # Speckles
    for sx, sy in [(5,5),(12,9),(20,6),(8,22),(24,18),(15,25),(27,10)]:
        d.point([(ox+sx, oy+sy)], fill=hl)


def draw_plank_tile(d: ImageDraw.ImageDraw, ox: int, oy: int,
                    top: bool = True) -> None:
    """Wooden plank tile."""
    base = PLANK_MID if top else PLANK_DARK
    d.rectangle([ox, oy, ox+31, oy+31], fill=base)
    # Plank grain lines
    for y in range(oy+4, oy+32, 8):
        d.line([(ox, y), (ox+31, y)], fill=PLANK_DARK if top else STONE_DARK, width=1)
    # Top edge highlight
    if top:
        d.line([(ox, oy), (ox+31, oy)], fill=PLANK_LIGHT, width=3)
        # Nail dots
        d.ellipse([ox+4, oy+12, ox+8, oy+16], fill=STONE_DARK)
        d.ellipse([ox+23, oy+12, ox+27, oy+16], fill=STONE_DARK)


def draw_dirt_tile(d: ImageDraw.ImageDraw, ox: int, oy: int) -> None:
    d.rectangle([ox, oy, ox+31, oy+31], fill=DIRT_MID)
    for sx, sy in [(3,3),(10,7),(18,4),(6,15),(22,12),(14,22),(28,18),(9,28)]:
        d.ellipse([ox+sx, oy+sy, ox+sx+3, oy+sy+3], fill=DIRT_DARK)


def draw_grass_tile(d: ImageDraw.ImageDraw, ox: int, oy: int) -> None:
    d.rectangle([ox, oy, ox+31, oy+31], fill=DIRT_MID)
    # Grass top strip
    d.rectangle([ox, oy, ox+31, oy+7], fill=GRASS_GREEN)
    # Blades
    for bx in range(ox+2, ox+31, 5):
        d.polygon([(bx, oy+7), (bx+2, oy+1), (bx+4, oy+7)], fill=(90,180,60,255))


def make_tileset() -> None:
    img = Image.new("RGBA", (CELL * COLS, CELL * ROWS), TRANS)
    d   = ImageDraw.Draw(img)

    # Row 0
    draw_stone_tile(d,        0,     0, STONE_MID, STONE_DARK, STONE_HL)   # GID 1 stone
    draw_stone_tile(d,     CELL,     0, STONE_LIGHT, STONE_MID, STONE_HL)  # GID 2 stone light
    draw_plank_tile(d,  2*CELL,     0, top=True)                            # GID 3 platform top
    draw_plank_tile(d,  3*CELL,     0, top=False)                           # GID 4 platform bottom

    # Row 1
    draw_stone_tile(d,        0, CELL, STONE_DARK, STONE_MID, STONE_LIGHT) # GID 5 wall dark
    draw_stone_tile(d,     CELL, CELL, STONE_MID, STONE_DARK, STONE_HL)    # GID 6 wall mid
    draw_dirt_tile (d,  2*CELL, CELL)                                       # GID 7 dirt
    draw_grass_tile(d,  3*CELL, CELL)                                       # GID 8 grass

    out = OUT / "test_tileset.png"
    img.save(out, "PNG")
    print(f"Generated: {out}  ({CELL*COLS}x{CELL*ROWS})")


if __name__ == "__main__":
    make_tileset()
    print("Done.")
