# Sessão 05 — M4: Inimigos IA FSM + Tiled Loader + Paralaxe + raycastAny

**Data:** 2026-05-17 / 2026-05-18  
**Milestone:** M4 — vertical slice jogável  
**Status final:** ✅ build limpo, 144 FPS, shutdown limpo

---

## O que foi construído

M4 vira a chave de "demo técnica" para "vertical slice jogável". O jogo agora tem um nível carregado por dados externos, três tipos de inimigo com IA reativa, e um fundo com paralaxe que dá profundidade visual.

### PhysicsWorld::raycastAny()

**Motivação:** sem uma primitiva de raycast, inimigos "veem através de paredes" — eles perceberiam o player mesmo sem linha de visão, quebrando a ilusão de mundo físico. A mesma função abre espaço para projetis com hitscam, lasers, e qualquer mecânica de percepção futura.

**Decisão técnica:** espelhar o padrão existente de `isOnGround()` (callback inline `b2RayCastCallback`). `return 0.f` no `ReportFixture` para no primeiro hit (comportamento "any"), ignora sensors (hitboxes não bloqueiam LoS) e ignora o próprio corpo do inimigo (`skip` param).

```cpp
RaycastHit PhysicsWorld::raycastAny(glm::vec2 from, glm::vec2 to, b2Body* skip) const
```

### Collision Categories (CollisionCategories.h)

**Motivação:** sem filtros de categoria, a hitbox do Walker bateria na hurtbox de outro Walker — inimigos se matariam entre si. Também impede que projéteis de inimigo acertem outros inimigos.

**Decisão técnica:** 6 categorias (`kCatWorld`, `kCatPlayer`, `kCatEnemy`, `kCatPlayerAttack`, `kCatEnemyAttack`, `kCatProjectile`) com máscaras pré-calculadas. Introduzido aqui em M4, aplicado retroativamente no player.

### TileMap loader (pugixml)

**Motivação:** `buildLevel()` hard-coded em C++ não escala. M5-M7 terão biomas diferentes — sem um pipeline de mapa, cada nível exige recompilação.

**Decisão técnica:** pugixml (sistema com FetchContent fallback) para parsear XML Tiled. Formato CSV encoding (sem base64/zlib) — simples de debugar e suficiente para os mapas que temos. Colisões via object layer "collision" (não decodificadas do tile grid) — mais flexível e menos código.

**Coordenadas:** Tiled usa Y-down pixels. Conversão `tiledToWorld()`: `cy = (mapHeightPx - pos.y - size.y*0.5) / ppm`. Offset `kMapOrigin = {-20, -2}` alinha o chão do TMX com `y=0m` world (mesma referência de M0-M3).

**Mapa gerado proceduralmente** (`scripts/gen_test_tmx.py`) — sem dependência do editor Tiled instalado. Usuário pode editar no Tiled depois sem quebrar o loader (desde que não use GID flips, que não são suportados nesta versão).

### TileMapRenderer

**Motivação:** renderizar tiles sem novo shader ou pipeline. SpriteBatch já suporta UV sub-rect.

**Decisão técnica:** iterar grid, calcular `(localId % cols, localId / cols)` para obter coluna/linha no atlas, computar uvMin/uvMax normalizados, chamar `batch.draw()`. Tile GID 0 = vazio, pular.

### ParallaxRenderer (3 camadas)

**Motivação:** paralaxe dá profundidade visual sem shaders avançados (Bloom/God Rays ficam para M5). É o detalhe visual mais perceptível por espectadores.

**Decisão técnica:** `layerX = camPos.x * (1 - factorX)` — fator 0=fixo, 1=rola com o mundo. `yOffsetFromCam` ao invés de `worldY` absoluto, porque as camadas devem seguir a câmera verticalmente (sempre na tela). Repetição horizontal (`rep = -2..+2`) com `quadW = tex->width / ppm` — seamless sem UV wrap complicado.

Ordem de render (painter's algorithm): parallax → tilemap → sprites. Sem depth test no SpriteBatch — a ordem de chamada define a sobreposição.

### EnemyAI FSM (3 tipos)

**Motivação:** o training dummy de M3 era estático. Para ser um vertical slice jogável o mundo precisa de entidades que reajam ao player.

#### Walker (slime-knight vermelho)
- Patrulha entre `patrolMinX/patrolMaxX`, alterna direção nas bordas
- Raycast horizontal para LoS — não percebe player atrás de parede
- Chase: vai ao player em velocidade 1.5× da patrulha
- Attack: fica parado, hitbox ativa em 0.05s–0.25s do estado, janela total 0.4s
- Hurt: 0.3s de stagger, volta ao Chase

#### Flyer (morcego roxo)
- Oscilação senoidal em Y durante Patrol (`sin(sinePhase) * 2.0f`)
- Chase: voa diretamente em 8 direções em velocidade 2×
- Ignora obstáculos para LoS (voa acima das paredes)
- Sem gravidade (`SetGravityScale(0)`)

#### Ranged (goblin arqueiro verde)
- Fica parado, sempre encarando o player
- Raycast para LoS, dispara projétil via callback `SpawnProjectileFn`
- Projétil: dynamic body sem gravidade, `Hitbox` sensor sempre ativo, `Projectile` component com lifetime
- `ProjectileSystem` decrementa lifetime e destrói corpo + entidade

**Padrão de clip swap:** espelha `PlayerControllerSystem.cpp` — `transitionState()` reseta `stateTimer`, busca clip por `enemyClipName(kind, state)`, aplica no `Animator`. Mapper separado evita switch aninhado espalhado.

**Cleanup pós-mortal:** entidades Dead acumulam `stateTimer` em `enemyAIUpdate()` e são destruídas após `kDeadDespawnDelay = 1.2s`, fora do loop de iteração para evitar invalidação de iterador.

### EnemyCombatSystem

**Motivação:** espelhar `combatPreUpdate` do player sem tocar no caminho do player que já estava validado.

**Decisão técnica:** `enemyCombatPreUpdate()` separado, itera `view<EnemyAI, Hitbox>`, gatea `hitbox.active` pela janela de timing do ataque. Box2D 2.4 não permite mudança de shape em fixture viva — hitbox baked na direção de spawn, flip por `categoryBits` na iteração.

### Spawns via TMX

**Motivação:** o último passo para conteúdo 100% data-driven — player e inimigos nascem de objetos no `.tmx`, não de código hard-coded.

**Implementação:** `spawnEnemyFromObject()` lê `obj.type` ("enemy_walker", "enemy_flyer", "enemy_ranged"), usa `obj.properties` para `patrolRange` e configura todos os parâmetros de IA. Projétil spawna com tint laranja e sem gravidade.

---

## Resultados de validação

- Build limpo: 0 warnings com `-DSTRICT_WARNINGS=ON`  
- FPS: 143-144 com 3 inimigos ativos (meta: ≥ 120)  
- Todas as texturas carregadas: player, dummy, 3 inimigos, tileset, 3 paralaxe  
- TileMap: `40×20 tiles | 1 tile layers | 2 object layers`  
- Shutdown: `[INFO] Shutdown clean`

---

## Arquivos criados/modificados

**Engine (novos):**
- `engine/include/engine/physics/CollisionCategories.h`
- `engine/include/engine/map/TileMap.h` + `engine/src/map/TileMap.cpp`
- `engine/include/engine/map/TileMapRenderer.h` + `engine/src/map/TileMapRenderer.cpp`
- `engine/include/engine/render/ParallaxRenderer.h` + `engine/src/render/ParallaxRenderer.cpp`

**Engine (modificados):**
- `engine/include/engine/physics/PhysicsWorld.h` — struct `RaycastHit` + declaração `raycastAny`
- `engine/src/physics/PhysicsWorld.cpp` — implementação `raycastAny`
- `engine/CMakeLists.txt` — novos `.cpp` + pugixml

**CMake:**
- `cmake/Dependencies.cmake` — pugixml com FetchContent fallback

**Game (novos):**
- `game/src/components/EnemyAI.h`
- `game/src/components/Projectile.h`
- `game/src/systems/EnemyAISystem.h/.cpp`
- `game/src/systems/EnemyCombatSystem.h/.cpp`
- `game/src/systems/ProjectileSystem.h/.cpp`

**Game (modificados):**
- `game/src/scenes/GameScene.h` — membros M4
- `game/src/scenes/GameScene.cpp` — buildLevel, update, render, clips de inimigos
- `game/CMakeLists.txt` — novos sistemas + cópia de maps

**Assets (gerados via Python):**
- `scripts/gen_tileset.py` → `assets/tilesets/test_tileset.png`
- `scripts/gen_parallax.py` → 3 PNGs de paralaxe
- `scripts/gen_test_tmx.py` → `maps/test_level.tmx`
- `scripts/gen_enemy_sheets.py` → 3 sprite sheets de inimigos (128×128, 4×4 células)
- 15 arquivos JSON de clips de animação em `assets/data/`

---

## Bugs encontrados e corrigidos

1. `'toB2' was not declared` — faltava `#include "engine/physics/PhysicsConstants.h"` no GameScene.cpp
2. `unused parameter 'dt'` — `float dt` → `float /*dt*/` em EnemyCombatSystem
3. `unused parameter 'physics'` — mesmo fix em updateFlyer
4. `variable 'dir' set but not used` — removida variável redundante em updateRanged
5. `ValueError: y1 >= y0` no PIL — confusão `ox` (x-offset) com `oy` (y-offset) em coordenadas Y dos sprites gerados

---

## Próximas etapas (M5)

- Bloom pass pós-processamento (framebuffer separado + blur + composição)
- God Rays / Fog via shader GLSL
- Sistema de partículas (death burst, trail de projétil)
- Mais variantes de inimigo e mecânicas (escudo, teleporte)
