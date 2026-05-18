#!/usr/bin/env python3
"""
Repack a downloaded enemy sprite pack into the Game2 enemy sheet layout.

Usage: python3 scripts/repack_enemy_sheet.py <extracted_pack_dir> [walker|flyer|ranged]

Output: assets/sprites/enemy_{type}.png  (4 cols × 5 rows, 32×32 frames)
  Row 0 — Patrol/Idle
  Row 1 — Chase/Run
  Row 2 — Attack
  Row 3 — Hurt
  Row 4 — Dead
"""

import pathlib, sys
from PIL import Image

FRAME_W, FRAME_H = 32, 32
COLS, ROWS       = 4, 5

ANIM_KEYS = {
    "walker": [
        ("patrol", ["walk", "patrol", "idle"]),
        ("chase",  ["run",  "chase"]),
        ("attack", ["attack", "slash", "hit"]),
        ("hurt",   ["hurt",  "damage"]),
        ("dead",   ["dead",  "death"]),
    ],
    "flyer": [
        ("patrol", ["fly",   "patrol", "idle"]),
        ("chase",  ["chase", "fly"]),
        ("attack", ["attack"]),
        ("hurt",   ["hurt"]),
        ("dead",   ["dead", "death"]),
    ],
    "ranged": [
        ("idle",   ["idle",   "patrol"]),
        ("attack", ["attack", "shoot", "throw"]),
        ("hurt",   ["hurt"]),
        ("dead",   ["dead", "death"]),
    ],
}

def find_sheet(base: pathlib.Path, keys: list[str]) -> pathlib.Path | None:
    for p in sorted(base.rglob("*.png")):
        name = p.name.lower()
        if any(k in name for k in keys):
            return p
    return None

def repack(base: pathlib.Path, enemy_type: str) -> Image.Image:
    sheet = Image.new("RGBA", (FRAME_W * COLS, FRAME_H * ROWS), (0, 0, 0, 0))
    rows  = ANIM_KEYS.get(enemy_type, ANIM_KEYS["walker"])

    for row_idx, (label, keys) in enumerate(rows):
        src = find_sheet(base, keys)
        if not src:
            print(f"  ⚠ '{label}' not found")
            continue
        raw    = Image.open(src).convert("RGBA")
        n_cols = max(1, raw.width // max(1, raw.height))
        n_cols = min(n_cols, 16)
        fw     = raw.width // n_cols
        for col in range(min(n_cols, COLS)):
            frame = raw.crop((col * fw, 0, (col + 1) * fw, raw.height))
            frame = frame.resize((FRAME_W, FRAME_H), Image.LANCZOS)
            sheet.paste(frame, (col * FRAME_W, row_idx * FRAME_H))
        print(f"  ✓ {label:8s} — {src.name}")

    return sheet

def main(pack_dir: str, enemy_type: str = "walker"):
    base = pathlib.Path(pack_dir)
    repo = pathlib.Path(__file__).resolve().parent.parent
    out  = repo / "assets" / "sprites" / f"enemy_{enemy_type}.png"

    sheet = repack(base, enemy_type)
    sheet.save(out)
    print(f"\n✓ enemy_{enemy_type}.png → {out}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <pack_dir> [walker|flyer|ranged]")
        sys.exit(1)
    t = sys.argv[2] if len(sys.argv) > 2 else "walker"
    main(sys.argv[1], t)
