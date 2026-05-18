#!/usr/bin/env python3
"""
Generate maps/test_level.tmx — Tiled-compatible XML map for M4.

Map layout: 40 columns × 20 rows, 32×32 tiles, CSV encoding.
mapOriginWorld in the game = (-20, -1) so the bottom of the map sits
at world y = -1, making the ground-top surface at world y = 0 (same
as M0-M3 hard-coded geometry).

Tile GIDs used (from test_tileset.png):
  1 = stone ground
  3 = platform top (wooden plank)
  0 = empty

Object layers:
  "collision" — static AABB boxes for Box2D
  "spawns"    — player and enemy spawn points (point objects with width=height=0)
"""
from pathlib import Path
import textwrap

MAP_COLS      = 40
MAP_ROWS      = 20
TILE_SIZE     = 32
MAP_W_PX      = MAP_COLS * TILE_SIZE   # 1280
MAP_H_PX      = MAP_ROWS * TILE_SIZE   # 640
OUT_DIR       = Path(__file__).parent.parent / "maps"
OUT_DIR.mkdir(parents=True, exist_ok=True)

STONE  = 1  # GID 1
PLANK  = 3  # GID 3 (platform plank)
EMPTY  = 0


def build_grid() -> list[list[int]]:
    """Return grid[row][col] with tile GIDs. row=0 is TOP (Tiled convention)."""
    grid = [[EMPTY] * MAP_COLS for _ in range(MAP_ROWS)]

    # Ground — bottom two rows (rows 18-19)
    for col in range(MAP_COLS):
        grid[18][col] = STONE
        grid[19][col] = STONE

    # Platform 1 — row 14, cols 8-12 (5 tiles)
    for col in range(8, 13):
        grid[14][col] = PLANK

    # Platform 2 — row 10, cols 18-22 (5 tiles)
    for col in range(18, 23):
        grid[10][col] = PLANK

    # Platform 3 — row 7, cols 28-33 (6 tiles)
    for col in range(28, 34):
        grid[7][col] = PLANK

    # Wall pillar between platforms (creates line-of-sight blocker)
    for row in range(14, 18):
        grid[row][15] = STONE   # col 15

    return grid


def csv_data(grid: list[list[int]]) -> str:
    """Encode grid as a CSV string (Tiled format)."""
    rows = []
    for r, row in enumerate(grid):
        suffix = "" if r == len(grid) - 1 else ","
        rows.append(",".join(str(g) for g in row) + suffix)
    return "\n".join(rows)


# ---------------------------------------------------------------------------
# Object helpers
# ---------------------------------------------------------------------------

_obj_id = 1

def box_obj(name: str, obj_type: str, x: int, y: int, w: int, h: int) -> str:
    global _obj_id
    oid = _obj_id; _obj_id += 1
    return (f'   <object id="{oid}" name="{name}" type="{obj_type}" '
            f'x="{x}" y="{y}" width="{w}" height="{h}"/>')


def point_obj(name: str, obj_type: str, x: int, y: int) -> str:
    global _obj_id
    oid = _obj_id; _obj_id += 1
    return (f'   <object id="{oid}" name="{name}" type="{obj_type}" '
            f'x="{x}" y="{y}"/>')


def build_tmx(grid: list[list[int]]) -> str:
    csv = csv_data(grid)

    # ── Collision boxes (Tiled pixel coords, Y from top) ───────────────────
    # Ground: rows 18-19 → y=576, h=64
    # Platforms: h=32 each
    # Side walls: x=-32 and x=1280, width=32 (off-map guards)
    collision_objs = "\n".join([
        box_obj("ground",     "solid",  0,    576, 1280, 64),
        box_obj("plat1",      "solid",  256,  448,  160, 32),  # cols 8-12, row 14
        box_obj("plat2",      "solid",  576,  320,  160, 32),  # cols 18-22, row 10
        box_obj("plat3",      "solid",  896,  224,  192, 32),  # cols 28-33, row 7
        box_obj("wall_left",  "solid",  -32,    0,   32, 640),
        box_obj("wall_right", "solid", 1280,    0,   32, 640),
        box_obj("pillar",     "solid",  480,  448,   32, 192),  # col 15, rows 14-19
    ])

    # ── Spawn points (y = pixel from top, will land above their floor) ─────
    # Player: col 2, row 16 (2 tiles above ground top)
    # Walker: col 10, row 16 (near ground, right of pillar)
    # Flyer:  col 20, row 8  (floating in open air)
    # Ranged: col 30, row 16 (right side, near platform 3)
    spawn_objs = "\n".join([
        point_obj("player_spawn", "player_spawn",  64,  512),   # col 2, row 16
        point_obj("walker1",      "enemy_walker",  320, 512),   # col 10, row 16
        point_obj("flyer1",       "enemy_flyer",   640, 256),   # col 20, row 8
        point_obj("ranged1",      "enemy_ranged",  960, 512),   # col 30, row 16
    ])

    return textwrap.dedent(f"""\
        <?xml version="1.0" encoding="UTF-8"?>
        <map version="1.10" tiledversion="1.10.2"
             orientation="orthogonal" renderorder="right-down"
             width="{MAP_COLS}" height="{MAP_ROWS}"
             tilewidth="{TILE_SIZE}" tileheight="{TILE_SIZE}"
             infinite="0" nextlayerid="4" nextobjectid="{_obj_id + 10}">

         <tileset firstgid="1" name="test_tileset"
                  tilewidth="{TILE_SIZE}" tileheight="{TILE_SIZE}"
                  tilecount="8" columns="4">
          <image source="../assets/tilesets/test_tileset.png"
                 width="128" height="64"/>
         </tileset>

         <layer id="1" name="tiles" width="{MAP_COLS}" height="{MAP_ROWS}">
          <data encoding="csv">
        {csv}
          </data>
         </layer>

         <objectgroup id="2" name="collision">
        {collision_objs}
         </objectgroup>

         <objectgroup id="3" name="spawns">
        {spawn_objs}
         </objectgroup>

        </map>
        """)


if __name__ == "__main__":
    grid = build_grid()
    tmx  = build_tmx(grid)
    out  = OUT_DIR / "test_level.tmx"
    out.write_text(tmx, encoding="utf-8")
    print(f"Generated: {out}")
    print(f"  Map size: {MAP_COLS}×{MAP_ROWS} tiles ({MAP_W_PX}×{MAP_H_PX} px)")
    print(f"  Tile layers: 1, Object layers: 2 (collision, spawns)")
    print("Done.")
