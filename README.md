# Game2 — Metroidvania 2.5D

Engine própria em C++20 + OpenGL 4.5 + SDL2. Sem Unity, sem Unreal, sem Godot.

## Dependências (instalar uma vez)

```bash
sudo dnf install -y \
  sdl2-compat-devel SDL2_image-devel \
  openal-soft-devel \
  assimp-devel \
  Box2D-devel \
  freetype-devel \
  pugixml-devel \
  glm-devel \
  glew-devel \
  mesa-libGL-devel mesa-libGLU-devel libglvnd-devel
```

Deps baixadas automaticamente pelo CMake (requer internet na primeira vez):
`GLM` (fallback), `stb`, `nlohmann/json`, `Dear ImGui`

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

## Rodar

```bash
./build/game/game
```

## Controles (M0 Bootstrap)

| Tecla | Ação |
|-------|------|
| ESC   | Sair |

## Milestones

| # | Nome | Status |
|---|------|--------|
| M0 | Bootstrap — janela + shader animado | ✅ |
| M1 | Renderer 2D + 3D (SpriteBatch, MeshRenderer, Camera) | 🔜 |
| M2 | ECS + Physics (Box2D) + Input | 🔜 |
| M3 | Animation + Combate + Câmera cinematográfica | 🔜 |
| M4 | Inimigos com IA FSM + Tiled (.tmx) | 🔜 |
| M5 | Shaders avançados (Bloom, Fog, God Rays, Partículas) | 🔜 |
| M6 | Áudio + UI + Save/Load + Mini-boss | 🔜 |
| M7 | Polish + Assets finais + Color grade | 🔜 |

## Estrutura

```
engine/   — biblioteca C++ agnóstica ao jogo
game/     — código específico do jogo (sistemas, cenas, componentes)
shaders/  — GLSL (vertex + fragment shaders)
assets/   — sprites, modelos, texturas, fontes
audio/    — música e SFX
maps/     — mapas Tiled (.tmx)
docs/     — documentação técnica
```

## Visual target

Ori and the Blind Forest · Hollow Knight · Ender Lilies · INSIDE · Dead Cells
