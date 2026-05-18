# Sessão 03 — 2026-05-17

**Disciplina:** Programação com Agentes  
**Objetivo:** Implementação do M2 — ECS + Physics (Box2D) + Input  
**Agente:** Claude Code (claude-sonnet-4-6)  
**Milestone anterior:** M1 ✅ (Renderer 2D+3D com SpriteBatch, MeshRenderer, Camera)  
**Milestone desta sessão:** M2 ✅ (ECS, Box2D, InputManager — demo de plataforma)

---

## Por que M2?

Depois de M0 (window + shader) e M1 (rendering 2D+3D), o jogo ainda não tinha
gameplay. A engine renderizava objetos, mas não sabia o que eram — era só um
canvas. Para ter um personagem que se move e colide, precisávamos de três
infraestruturas fundamentais:

1. **ECS (Entity Component System)**: forma de organizar "coisas do jogo" (player,
   inimigos, tiles) como conjuntos de dados em vez de hierarquias de classes. Isso
   é a espinha dorsal de qualquer game engine moderna — sem ele, cada sistema precisaria
   de seus próprios arrays de objetos, gerando acoplamento e dificuldade de extensão.

2. **Box2D**: motor de física 2D que simula gravidade, colisão entre corpos rígidos,
   impulsos. Sem Box2D, teríamos que implementar manualmente detecção de colisão,
   resolução de impulsos, etc — semanas de trabalho. Box2D é battle-tested e já
   disponível no sistema (`/usr/lib64/libbox2d.so.2.4.1`).

3. **InputManager**: abstração entre teclas físicas e ações do jogo. Sem isso,
   o código dos sistemas de gameplay precisa saber os SDLScancodes específicos,
   dificultando rebinding e manutenção.

---

## Decisões técnicas tomadas

Antes de implementar, o agente (em Plan Mode) perguntou 3 questões-chave:

| Questão | Decisão | Raciocínio |
|---------|---------|------------|
| Visual do demo M2 | Plataforma side-view 2.5D | Mantém o pipeline 2.5D (perspectiva FoV 30°) evitando refactor para M3+. Player cai, anda, pula. |
| Box2D DebugDraw | Sim, toggle F1 | Essencial para validar formas de colisão visualmente. Sem isso, depurar física é cego. |
| Input | Teclado + action map | `InputManager` desacopla teclas de lógica. Gamepad pode ser adicionado depois sem tocar nos sistemas. |

---

## Arquitetura implementada

### ECS — Sparse-Set Registry

**Problema:** precisamos de um container que associe entidades a componentes de forma
eficiente. A solução clássica de jogos industriais (EnTT, Bevy ECS) usa *sparse sets*:

- **Sparse array** (`m_sparse[entity_id]`): índice no array denso, ou `~0u` se ausente.
- **Dense array** (`m_dense[]`): IDs das entidades que têm o componente, contíguos em memória.
- **Components array** (`m_components[]`): dados, na mesma ordem que `m_dense`.

Isso garante:
- `insert`/`erase`/`contains`: O(1) amortizado
- Iteração: acesso sequencial à memória (cache-friendly)
- Nenhuma indireção em runtime

O `Registry` usa type-erasure com `std::type_index → unique_ptr<IPool>`. Cada tipo
de componente ganha seu próprio `SparseSet<T>`. A função `view<Cs...>()` itera o menor
pool (o pivot) e filtra os outros com `has<C>(e)`.

Arquivos criados:
- `engine/include/engine/ecs/Entity.h` — `struct Entity { uint32_t id; uint32_t gen; }`
- `engine/include/engine/ecs/SparseSet.h` — template storage O(1)
- `engine/include/engine/ecs/Registry.h` — API pública (create/destroy/emplace/view)
- `engine/src/ecs/Registry.cpp` — free-list de IDs com geração

### InputManager — Action Map

**Problema:** o sistema de eventos do SDL entrega keycodes brutos. Vincular `SDL_SCANCODE_SPACE`
diretamente ao código de salto do player cria acoplamento que dificulta rebinding.

**Solução:** enum `Action` (MoveLeft, MoveRight, Jump, etc.) mapeado para scancodes.
`InputManager::beginFrame()` tira um snapshot do estado do teclado via `SDL_GetKeyboardState()`
e detecta transições (`pressed = curr && !prev`, `released = !curr && prev`).

Integrado no loop do `App` — `beginFrame()` antes do `SDL_PollEvent`, `handleEvent(ev)`
para cada evento. Isso garante que todos os sistemas recebam o estado correto no frame.

Arquivos criados:
- `engine/include/engine/input/Action.h` — enum class com 7 ações
- `engine/include/engine/input/InputManager.h/.cpp` — snapshot + transições
- `engine/include/engine/core/App.h` — getter `input()` adicionado
- `engine/src/core/App.cpp` — `beginFrame`/`handleEvent` no event pump

### PhysicsWorld — Box2D 2.4.x Wrapper

**Problema:** Box2D tem uma API direta mas verbosa. Cada corpo precisa de BodyDef,
FixtureDef, Shape — 10+ linhas para criar um retângulo estático.

**Solução:** `PhysicsWorld` expõe helpers diretos:
- `createStaticBox(pos, halfW, halfH)` — chão, paredes, plataformas
- `createDynamicBox(pos, halfW, halfH, density, friction)` — player, caixas
- `step(dt)` com acumulador de passo fixo 60 Hz (máx 5 substeps) para física determinística
- `isOnGround(body, halfH)` via raycast descendente — detecta se o player está no chão

**Passo fixo:** sem acumulador, a física variaria com FPS. Com acumulador, o mundo
sempre avança em steps de 1/60s, independente do framerate real. Isso evita o
"spiral of death" onde lag causa mais substeps, causando mais lag.

**Convenção:** `1 metro = 32 pixels` (constante `kPixelsPerMeter` em `PhysicsConstants.h`).
Mas como a câmera 2.5D também enxerga em metros (não em pixels), não há conversão em
runtime — o render usa metros diretamente.

### DebugDraw — Visualização de Colisores

**Problema:** sem visualizar os corpos físicos, depurar colisão é impossível.

**Solução:** `DebugDraw` subclassa `b2Draw` (interface do Box2D) e coleta segmentos de
linha de todos os polígonos/círculos na simulação. No fim do frame (se F1 ativo),
faz upload para um VBO dinâmico e desenha com `GL_LINES` usando um shader flat.

Shader `debug_line.vert/frag`: recebe `vec3 pos + vec3 color`, aplica `uViewProj`,
saída flat. Simples e eficiente.

### GameScene — Orquestração

**Problema:** o `App` só sabe de loop de frames. Quem organiza os sistemas?

**Solução:** `GameScene` possui `Registry`, `PhysicsWorld`, `SpriteBatch`, `DebugDraw`
e executa os sistemas na ordem correta a cada frame:

```
Input.beginFrame()  [no App]
↓
playerControllerUpdate(reg, input, physics)
  → isOnGround? → ajusta velocidade X, aplica impulso de salto
↓
physics.step(dt)
  → atualiza posições Box2D
↓
physicsSyncUpdate(reg)
  → copia b2Body.position → Transform.position
↓
camera follow  (lerp X do player)
↓
spriteRenderUpdate(reg, batch)
  → desenha sprites de todas entidades com Transform+SpriteRenderer
↓
[se F1] debugDraw.render(camera)
```

Arquivos criados:
- `game/src/scenes/GameScene.h/.cpp` — orquestração
- `game/src/components/{Transform,RigidBody,SpriteRenderer,PlayerControl}.h` — PODs
- `game/src/systems/{PlayerControllerSystem,PhysicsSyncSystem,SpriteRenderSystem}.h/.cpp`

---

## Layout da cena de demo

```
y
^
|
5 ─  [wall L]              [wall R]
4 ─  |                           |
3 ─  |      [player]             |
2 ─  |         ◻                 |
1 ─  |                           |
0 ══════════════════════════════════  (chão)
    -8                           +8  → x (metros)
```

| Corpo | Tipo | Centro | Half-extents |
|-------|------|--------|--------------|
| Chão  | Static | (0, −0.5) | (8, 0.5) |
| Parede esq | Static | (−8, 4) | (0.12, 5) |
| Parede dir | Static | (+8, 4) | (0.12, 5) |
| Player | Dynamic | (0, 2) | (0.25, 0.5) |

Câmera: perspectiva FoV 30° em z=16m, seguindo player no eixo X com lerp (5×dt).

---

## Resultado da validação

```
[INFO] OpenGL  : 4.6 (Core Profile) Mesa 25.3.6
[INFO] Texture loaded: '…/assets/sprites/test.png' [32x32 ch=4]
[INFO] M2 ready — A/D move | Space jump | F1 debug | ESC quit
[INFO] FPS: 144  |  frame: 6.95 ms
[INFO] Shutdown clean
```

Checks visuais confirmados:
- Player sprite aparece na cena e cai por gravidade até o chão
- A/D movem horizontalmente; paredes impedem saída dos limites
- Espaço pula (único pulo — sem double-jump ainda)
- F1 exibe retângulos de colisão Box2D em wireframe verde

---

## Bug encontrado e corrigido durante implementação

**Erro:** `'struct eng::ecs::Registry::Pool<T>' has no member named 'insert'`

**Causa:** `pool<C>()` retorna `Pool<C>&` (o wrapper type-erased), não `SparseSet<C>&`.
A chamada correta é `pool<C>().data.insert(...)` acessando o membro `.data` público.

**Correção:** `pool<C>().insert(...)` → `pool<C>().data.insert(...)` em `Registry.h:31`.

---

## Arquivos criados nesta sessão (26 novos + 7 modificados)

| Arquivo | Tipo |
|---------|------|
| `engine/include/engine/ecs/{Entity,SparseSet,Registry}.h` | Novo |
| `engine/src/ecs/Registry.cpp` | Novo |
| `engine/include/engine/input/{Action,InputManager}.h` | Novo |
| `engine/src/input/InputManager.cpp` | Novo |
| `engine/include/engine/physics/{PhysicsConstants,PhysicsWorld,DebugDraw}.h` | Novo |
| `engine/src/physics/{PhysicsWorld,DebugDraw}.cpp` | Novo |
| `shaders/debug_line.{vert,frag}` | Novo |
| `game/src/components/{Transform,RigidBody,SpriteRenderer,PlayerControl}.h` | Novo |
| `game/src/systems/{PlayerControllerSystem,PhysicsSyncSystem,SpriteRenderSystem}.{h,cpp}` | Novo |
| `game/src/scenes/GameScene.{h,cpp}` | Novo |
| `engine/include/engine/core/App.h` | Modificado (input accessor) |
| `engine/src/core/App.cpp` | Modificado (beginFrame + handleEvent) |
| `engine/CMakeLists.txt` | Modificado (novos .cpp, box2d PUBLIC) |
| `game/CMakeLists.txt` | Modificado (novos .cpp, include_dirs) |
| `cmake/Dependencies.cmake` | Modificado (box2d REQUIRED) |
| `game/src/main.cpp` | Modificado (GameApp → GameScene) |
| `README.md` | Modificado (controles, M2 ✅, box2d-devel) |

---

## Próximos passos (M3)

- **Animator**: `SpriteAnimation` com array de frames e tempos; `AnimationStateMachine`
  (Idle/Run/Jump/Fall via transições baseadas em velocity + grounded)
- **FSM do player**: estados explícitos com callbacks de entrada/saída
- **Coyote time** (~100 ms) e **jump buffer** (~150 ms) — feel essencial de plataforma
- **Câmera look-ahead**: desloca câmera levemente na direção da velocidade do player
- **Screen shake**: modelo de trauma (Squirrel Eiserloh GDC) para hits futuros
