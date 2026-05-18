#!/usr/bin/env python3
"""
Repack a downloaded Craftpix / itch.io player sprite pack into the
player_sheet.png layout expected by Game2 (4×5 grid, 32×32 frames).

Usage: python3 scripts/repack_player_sheet.py <extracted_pack_dir>

Expected frame layout after repacking:
  Row 0 — Idle   (4 frames)
  Row 1 — Run    (4 frames)
  Row 2 — Jump   (4 frames)
  Row 3 — Fall   (4 frames)
  Row 4 — Attack (4 frames)
"""

import pathlib, sys
from PIL import Image

FRAME_W, FRAME_H = 32, 32
COLS, ROWS       = 4, 5

def find_sheet(base: pathlib.Path, keywords: list[str]) -> pathlib.Path | None:
    for p in sorted(base.rglob("*.png")):
        name = p.name.lower()
        if any(k in name for k in keywords):
            return p
    return None

def extract_frames(sheet: Image.Image, n: int, row: int = 0) -> list[Image.Image]:
    fw = sheet.width // n
    fh = sheet.height
    return [sheet.crop((i * fw, row * fh, (i + 1) * fw, (row + 1) * fh)).resize(
                (FRAME_W, FRAME_H), Image.LANCZOS)
            for i in range(min(n, COLS))]

def pad_frames(frames: list[Image.Image], target: int) -> list[Image.Image]:
    while len(frames) < target:
        frames.append(frames[-1] if frames else Image.new("RGBA", (FRAME_W, FRAME_H)))
    return frames[:target]

def main(pack_dir: str):
    base = pathlib.Path(pack_dir)
    repo = pathlib.Path(__file__).resolve().parent.parent
    out  = repo / "assets" / "sprites" / "player_sheet.png"

    sheet = Image.new("RGBA", (FRAME_W * COLS, FRAME_H * ROWS), (0, 0, 0, 0))

    mappings = [
        ("idle",   ["idle"]),
        ("run",    ["run"]),
        ("jump",   ["jump"]),
        ("fall",   ["fall"]),
        ("attack", ["attack", "slash"]),
    ]

    for row, (label, keys) in enumerate(mappings):
        src = find_sheet(base, keys)
        if not src:
            print(f"  ⚠ No sheet found for '{label}' — using blank row")
            frames = [Image.new("RGBA", (FRAME_W, FRAME_H)) for _ in range(COLS)]
        else:
            raw    = Image.open(src).convert("RGBA")
            n_cols = raw.width // (raw.height if raw.height > 0 else 1)
            n_cols = max(1, min(n_cols, 16))
            frames = extract_frames(raw, n_cols)
            frames = pad_frames(frames, COLS)
            print(f"  ✓ {label:8s} — {src.name}")

        for col, frame in enumerate(frames):
            sheet.paste(frame, (col * FRAME_W, row * FRAME_H))

    sheet.save(out)
    print(f"\n✓ player_sheet.png saved → {out}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <extracted_pack_dir>")
        sys.exit(1)
    main(sys.argv[1])
