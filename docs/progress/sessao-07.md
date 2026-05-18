# M7 — Polish, Color Grade, Menus & Bug Fixes
**Data:** 2026-05-18  
**Milestone:** M7 (final)

---

## Objetivo

Fechar o jogo com qualidade de indie comercial: menus completos, fluxo de cenas, color grade cinematográfica, correção de bugs de combate reportados após M6, e melhoria dos sprites placeholder.

---

## Fase A — Bug fix de combate (completo)

### A.1 — Alinhar SpriteRenderer ao body
- Player: `{0.5f, 1.0f}` (era `{0.9, 0.9}`)
- Enemy/Boss: `halfW*2 × halfH*2` (era 1.4×/1.2× maior que o body)
- Eliminou o transbordamento visual de sprites sobre hurtboxes invisíveis

### A.2 — AttackTable (hitbox data-driven)
- **NOVO** `engine/include/engine/animation/AttackTable.h` + `.cpp`
- Structs `AttackWindow` (start/end/offset/halfSize/damage/knockback) e `AttackData`
- Carregado de `assets/data/player_attacks.json` (single source of truth)
- `CombatSystem.cpp`: removidas 3 constantes hardcoded (`kHitboxOffsetX/HalfW/HalfH`); janela de hitbox agora usa `w.start`/`w.end` do JSON (mais precisão), sensor sempre reposicionado pelo `facing`

### A.3 — flipX runtime da hitbox de inimigos e boss
- `EnemyCombatSystem.cpp`: substituiu bloco morto `(void)shape` por `dynamic_cast<b2PolygonShape*>()->SetAsBox(...)` com `facingSign` em runtime
- `BossLogic.cpp`: mesmo fix — hitbox do boss agora aponta para o lado correto

### A.4 — Knockback de inimigos
- `CombatSystem.cpp`: substituiu `PlayerControl::facing` fixo por cadeia `EnemyAI::facing` → vetor posicional de fallback
- Knockback agora empurra sempre na direção correta independente de quem atacou

### A.5 — Sensor fora do ataque
- Sensor reposicionado toda frame (não só quando `inAttack=true`)

---

## Fase B — SceneManager + Menus (completo)

### B.1 — SceneId + MenuAction
- **NOVO** `game/src/scenes/SceneId.h`: `enum class SceneId { MainMenu, Playing, Paused, GameOver, Settings }` e `enum class MenuAction { ... }`

### B.2 — MenuScene
- **NOVO** `game/src/scenes/MenuScene.h/.cpp`
- 4 páginas: Main / Pause / GameOver / Settings
- Navegação por teclado com repetição de tecla (150 ms inicial + 80 ms repeat)
- Sliders de volume Master / Music / SFX com ajuste em tempo real via `AudioEngine`
- Settings persistidos em `SaveSystem` ao pressionar Esc/Back
- `render(isOverlay)`: overlay escuro semi-transparente para Pause/GameOver

### B.3 — SceneManager em main.cpp
- `GameApp` refatorado: `SceneId m_current`, `unique_ptr<GameScene>`, `MenuScene m_menu`
- Transições: MainMenu → Playing, Playing → Paused (ESC), Paused → Playing/MainMenu, GameOver → Retry/MainMenu
- Retry: chama `GameScene::reinit()` que reconstrói o ECS sem recriar a física
- Pause: `AudioEngine::setMusicPaused(true/false)` sincronizado

### B.4 — Dead state + GameOver
- `PlayerState::Dead` adicionado ao enum
- `PlayerControllerSystem`: freeze horizontal, gravidade ativa, animação "hurt" no dead
- `GameScene`: `m_deathTimer` acumula; `playerDeadDelayElapsed()` retorna true após 1.5 s
- `main.cpp` transiciona para `SceneId::GameOver` automaticamente

### B.5 — Input Menu
- `Action.h`: `MenuUp/Down/Left/Right/Confirm/Cancel` adicionados antes de `kCount`
- `InputManager.cpp`: bindings `W/S/↑/↓/←/→/Return/Escape`; `Q` agora é Quit (ESC virou MenuCancel)

### B.6 — Font path robusto
- `Hud.cpp`: tenta `ASSET_ROOT/assets/fonts/Kenney_Future.ttf` → LiberationSans-Bold → DejaVuSans → Carlito
- `MenuScene.cpp`: mesma cadeia de fallback via `ifstream` check

### B.7 — Registry::clear()
- Adicionado `void clear()` ao `eng::ecs::Registry` (m_pools + generations + freeList reset)
- Necessário para `GameScene::reinit()` sem mover/copiar o Registry

---

## Fase C — Color grade LUT 3D (completo)

### C.1 — Texture3D
- **NOVO** `engine/include/engine/render/Texture3D.h` + `.cpp`
- Lê PNG em layout Hald (N²×N) e reorganiza em volume GL_TEXTURE_3D
- `GL_RGB8`, `GL_LINEAR` (interpolação trilinear automática), `GL_CLAMP_TO_EDGE`

### C.2 — composite.frag
- `uniform sampler3D uColorLUT`, `uLutStrength`, `uLutSize` adicionados
- `applyLUT(c)` com correção de half-texel offset
- Mix aplicado entre gamma e vinheta: `ldr = mix(ldr, applyLUT(ldr), uLutStrength)`

### C.3 — PostProcessStack
- `setLUT(shared_ptr<Texture3D>)` + `setLUTStrength(float)` adicionados ao header
- `doComposite()`: bind `GL_TEXTURE3` + set uniforms; fallback `uLutStrength=0` sem LUT

### C.4 — LUT gerada em Python
- **NOVO** `scripts/gen_lut.py`: pipeline cinematográfico (lift-gamma-gain teal/warm + saturação × 1.10 + S-curve suave)
- Output: `assets/textures/lut_cinematic.png` (64³, 74 KB)
- Wire-up: `GameScene::init()` carrega LUT com `setLUTStrength(0.85f)`

---

## Fase D — Sprites polished + scripts de download (completo)

### D.1 — Polished procedural player sheet
- **NOVO** `scripts/gen_player_sheet_polished.py`: anti-aliased, paleta de 7 cores, squash/stretch no jump/fall, sword swing animado, outline procedural
- Gerado `assets/sprites/player_sheet.png`

### D.2 — Scripts de download e repack
- **NOVO** `scripts/download_assets.sh`: instruções de download de Craftpix e itch.io + URL do Kenney Future font
- **NOVO** `scripts/repack_player_sheet.py`: repack genérico de pack externo → layout 4×5 esperado pelo game
- **NOVO** `scripts/repack_enemy_sheet.py`: repack de pack de inimigos → walker / flyer / ranged

### D.3 — .gitignore e README
- `.gitignore`: exceções para `lut_cinematic.png` e `Kenney_Future.ttf`
- `README.md`: controles atualizados (ESC=Pause, Q=Quit, setas menu), M7 ✅, seção Créditos com licenças

---

## Arquivos criados

| Arquivo | Propósito |
|---------|-----------|
| `engine/include/engine/animation/AttackTable.h` | AttackWindow / AttackData / AttackTable |
| `engine/src/animation/AttackTable.cpp` | Parser JSON |
| `engine/include/engine/render/Texture3D.h` | RAII GL_TEXTURE_3D |
| `engine/src/render/Texture3D.cpp` | Hald PNG → volume 3D |
| `game/src/scenes/SceneId.h` | SceneId + MenuAction enums |
| `game/src/scenes/MenuScene.h/.cpp` | Menu principal / pause / gameover / settings |
| `scripts/gen_lut.py` | Gera lut_cinematic.png |
| `scripts/gen_player_sheet_polished.py` | Player sheet procedural polido |
| `scripts/download_assets.sh` | Instruções de download |
| `scripts/repack_player_sheet.py` | Repack de pack externo |
| `scripts/repack_enemy_sheet.py` | Repack de inimigos |
| `docs/progress/sessao-07.md` | Este arquivo |

---

## Arquivos modificados (principais)

| Arquivo | Mudança |
|---------|---------|
| `game/src/main.cpp` | SceneId dispatch, unique_ptr, transições |
| `game/src/scenes/GameScene.h/.cpp` | reinit(), playerDeadElapsed(), death timer, LUT load |
| `game/src/systems/CombatSystem.cpp` | AttackTable, knockback fix |
| `game/src/systems/EnemyCombatSystem.cpp` | SetAsBox runtime |
| `game/src/systems/BossLogic.cpp` | SetAsBox boss hitbox |
| `game/src/systems/PlayerControllerSystem.cpp` | Dead state |
| `engine/include/engine/ecs/Registry.h` | clear() |
| `engine/include/engine/render/PostProcessStack.h` | setLUT/setLUTStrength |
| `engine/src/render/PostProcessStack.cpp` | bind Texture3D unit 3 |
| `shaders/composite.frag` | sampler3D + applyLUT |
| `engine/include/engine/input/Action.h` | MenuUp/Down/Left/Right/Confirm/Cancel |
| `engine/src/input/InputManager.cpp` | bindings |
| `game/src/ui/Hud.cpp` | font path robusto |
| `game/CMakeLists.txt` | MenuScene.cpp |
| `engine/CMakeLists.txt` | AttackTable.cpp, Texture3D.cpp |

---

## Resultado final

- **Build**: 0 erros, 1 warning pre-existente (BossLogic physics unused param)
- **Boot**: abre no Main Menu (não mais direto no jogo)
- **Fluxo**: MainMenu → New Game → Playing → ESC → Paused → Resume/Settings/MainMenu → GameOver → Retry/MainMenu
- **Color grade**: LUT 64³ com teal nas sombras e highlights quentes, 85% de strength
- **Combat bugs**: hitboxes alinhadas, flipX correto em runtime, knockback direcional
- **Save**: settings de volume persistidos; retry restaura HP/posição do save
