#!/usr/bin/env bash
# download_assets.sh — Download free art assets for Game2 M7.
#
# Run from repo root:  bash scripts/download_assets.sh
#
# Assets:
#   Font : Kenney Future (CC0) from kenney.nl
#   Note : Sprite packs from Craftpix require manual download (registration needed).
#          Repack scripts: scripts/repack_player_sheet.py, scripts/repack_enemy_sheet.py

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

echo "── Downloading Kenney Future font ────────────────────────────────"
mkdir -p "$REPO_ROOT/assets/fonts"
FONT_URL="https://raw.githubusercontent.com/nicholasgasior/kenney-fonts/master/fonts/Kenney%20Future.ttf"
curl -L --fail -o "$REPO_ROOT/assets/fonts/Kenney_Future.ttf" "$FONT_URL"
echo "✓ Font saved to assets/fonts/Kenney_Future.ttf"

echo ""
echo "── Manual sprite download instructions ───────────────────────────"
echo ""
echo "1. Player sprite (Hero Knight, CC0-like):"
echo "   https://craftpix.net/freebies/free-hero-knight-pixel-art-character-pack/"
echo "   → Download → extract → run: python3 scripts/repack_player_sheet.py <extracted_dir>"
echo ""
echo "2. Enemy sprites (Skeleton Enemies):"
echo "   https://craftpix.net/freebies/free-skeleton-enemy-sprite-sheets-pixel-art/"
echo "   → Download → extract → run: python3 scripts/repack_enemy_sheet.py <extracted_dir>"
echo ""
echo "3. Alternative CC0 sprites from itch.io:"
echo "   https://itch.io/game-assets/free/tag-platformer"
echo ""
echo "To generate polished procedural sprites as fallback:"
echo "   python3 scripts/gen_player_sheet_polished.py"
echo "   python3 scripts/gen_enemy_sheets.py  (already exists)"
