# Game2 — Metroidvania 2.5D

Engine própria em C++20 + OpenGL 4.5 + SDL2. Sem Unity, sem Unreal, sem Godot.

## Dependências (instalar uma vez)

```bash
sudo dnf install -y \
  sdl2-compat-devel SDL2_image-devel \
  openal-soft-devel \
  assimp-devel \
  box2d-devel \
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

## Controles

| Tecla | Ação |
|-------|------|
| A / ← | Mover esquerda |
| D / → | Mover direita |
| Espaço | Pular (coyote time + jump buffer) |
| J | Atacar (hitbox data-driven) |
| ESC | Pause / Voltar |
| ↑ / W | Navegar menu (cima) |
| ↓ / S | Navegar menu (baixo) |
| ← / → | Ajustar sliders de volume |
| Enter | Confirmar |
| F1 | Toggle debug-draw Box2D (hitboxes visíveis) |
| F5 | Salvar jogo (posição + HP + inimigos derrotados) |
| F9 | Carregar último save |
| Q | Sair |

**Mecânicas de movimento:**
- Soltar Espaço cedo = pulo curto (variable jump height)
- Pular até 100 ms após sair de uma borda = coyote time
- Pressionar Espaço até 150 ms antes de aterrissar = jump buffer

## Milestones

| # | Nome | Status |
|---|------|--------|
| M0 | Bootstrap — janela + shader animado | ✅ |
| M1 | Renderer 2D + 3D (SpriteBatch, MeshRenderer, Camera) | ✅ |
| M2 | ECS + Physics (Box2D) + Input | ✅ |
| M3 | Animation StateMachine + Combate data-driven + Câmera cinematográfica | ✅ |
| M4 | Inimigos com IA FSM + Tiled (.tmx) + Paralaxe + raycastAny | ✅ |
| M5 | Shaders avançados (Bloom, Fog, God Rays, Partículas) | ✅ |
| M6 | Áudio + UI + Save/Load + Mini-boss | ✅ |
| M7 | Polish + Color grade LUT + Menus + Bug fixes | ✅ |

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

## Créditos

| Asset | Autor | Licença |
|-------|-------|---------|
| Kenney Future (fonte) | Kenney.nl | CC0 — https://kenney.nl/assets/kenney-fonts |
| Liberation Sans Bold | Red Hat / Liberation | SIL OFL 1.1 |
| Carlito Bold | Google Fonts | SIL OFL 1.1 |
| stb_image | Sean Barrett | Public Domain / MIT |
| nlohmann/json | Niels Lohmann | MIT |
| Dear ImGui | Omar Cornut | MIT |
| GLM | G-Truc Creation | MIT |
| Box2D 2.4 | Erin Catto | MIT |
| OpenAL Soft | Chris Robinson | LGPL 2.1 |
| FreeType | FreeType Project | FreeType License |

Para sprites de terceiros (Craftpix / itch.io), veja `scripts/download_assets.sh` para instruções de download.
