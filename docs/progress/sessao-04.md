# Sessão 04 — M3: Animation StateMachine + Combat + Cinematic Camera

**Data:** 2026-05-17  
**Commit:** `feat: M3 Animation FSM + Combat (data-driven) + Cinematic camera`  
**Branch:** main

---

## Contexto e motivação

Após M2, o projeto tinha um cubo azul que se movia e pulava — impressionante para validar física e ECS, mas indistinguível de qualquer demo de colisão aleatória. M3 é o milestone que transforma o "esqueleto técnico" num jogo jogável: o player ganhou identidade visual (sprite sheet animado), o pulo ganhou *player feel* correto (coyote time + jump buffer), e o combate ganhou toda a infraestrutura data-driven necessária para as próximas milestones.

A câmera cinematográfica também foi prioridade aqui porque ela comunica ao jogador (e à banca) que o projeto tem intenção artística: look-ahead, deadzone, screen shake e zoom dinâmico em combate são as quatro técnicas que distinguem jogos indie polidos de protótipos acadêmicos.

---

## Decisões técnicas — por que cada escolha foi feita

### 1. `AnimationClip` na engine, `Animator` no jogo

`AnimationClip` é dado puro: nome, frames (uvMin/uvMax/duration), flag de loop. Não depende de ECS, de renderização, nem de estado de instância. Por isso mora em `engine/animation/` e pode ser reutilizado via `ResourceManager<AnimationClip>` sem nenhuma mudança de infra.

`Animator` é estado *por entidade*: qual clip está tocando, em que frame estamos, se já terminou. Ele mora em `game/src/components/` porque é específico ao ECS do jogo.

Essa separação mantém a engine agnóstica ao jogo — princípio central de todo o projeto.

### 2. UV sub-rect em `SpriteBatch` — por que não um `SpriteSheet` separado

Adicionar um overload `draw(..., uvMin, uvMax, flipX)` ao `SpriteBatch` existente foi mais simples do que criar uma classe `SpriteSheet`. A vertex struct já carregava `uv vec2` — foi uma mudança de 2 linhas no preenchimento das UVs. Custo zero de abstração extra.

O `flipX` troca os extremos U antes do push dos vértices, sem precisar de nenhuma matriz de transformação extra. Simples e eficiente.

### 3. State machine como `enum + switch`, não classes polimórficas

A FSM do player tem 6 estados: Idle, Run, Jump, Fall, Attack, Hurt. Criar uma classe por estado (padrão GoF State) adicionaria ~200 linhas de boilerplate virtual para zero ganho real — cada estado teria apenas 2–3 linhas de lógica. Um `switch(state)` inline no `PlayerControllerSystem` é legível, debugável por printf, e não tem overhead de virtual dispatch.

### 4. Coyote time + jump buffer como timers floats simples

Coyote time (100 ms) e jump buffer (150 ms) são implementados como `float` decrementados por `dt` a cada frame — não precisam de event queue nem de máquina de estados separada. O algoritmo:

```
Se saiu do chão → coyoteTimer = 0.10s
Se pressionou Jump → jumpBufferTimer = 0.15s
Se jumpBufferTimer > 0 e (grounded ou coyoteTimer > 0) → aplica pulo
```

Variable jump height: ao soltar Space com `vel.y > 0`, multiplicamos `vel.y * 0.5`. Resulta em tap curto = pulo baixo, hold = pulo cheio.

### 5. Hitboxes como sensores Box2D + `b2ContactListener`

Em vez de fazer nossa própria broadphase AABB, aproveitamos o Box2D que já está na cena. Sensores Box2D (isSensor = true) detectam overlaps mas não geram força de separação — exatamente o que uma hitbox precisa.

O `CombatContactListener::BeginContact` **nunca modifica o mundo** — apenas enfileira `HitEvent` em um `std::vector`. Modificar entidades dentro do callback do Box2D é UB (o world está em mid-step). Em `combatPostUpdate` (após `physics.step()`), drenamos a fila e aplicamos dano + knockback com segurança.

O gating de hits funciona via `Hitbox.active` (flag no componente) e `Hitbox.hitMask` (bitset de entidades já atingidas no swing atual). Não precisamos de `b2Fixture::SetEnabled()` — que não existe no Box2D 2.4.x do Fedora 43.

### 6. Hit-stop como `m_timeScale` global no GameScene

Hit-stop é a técnica de congelar brevemente o tempo ao acertar um inimigo — dá sensação de peso e impacto. Implementação: `GameScene::update(dt)` multiplica `dt * m_timeScale` antes de propagar para sistemas de gameplay. Quando um hit acontece, `m_timeScale = 0` por 60 ms.

A câmera usa o `dt` real (não escalado) para continuar animando durante o hit-stop. Isso é importante: shake e zoom devem acontecer *durante* o freeze para transmitir o impacto — se a câmera também congelasse, o efeito seria invisível.

### 7. Trauma model para screen shake (Squirrel Eiserloh, GDC 2015)

O modelo de trauma usa `trauma²` como intensidade do shake. Isso tem duas vantagens:
- **Não-linear**: pequenos valores de trauma (0.1–0.3) produzem shake imperceptível; apenas valores grandes (0.6+) produzem shake visível. Evita shake constante durante movimentação normal.
- **Composto**: múltiplos hits acumulam trauma até 1.0, gerando shake crescente para combate intenso.

O decaimento é linear (`trauma -= 1.5 * dt`), então um trauma de 0.65 some em ~0.43 segundos.

### 8. Look-ahead baseado em `facing`, não em `velocity`

A câmera se adianta na direção para onde o player está *olhando*, não na direção do movimento instantâneo. Por quê? A velocidade varia muito: oscila durante air-control, zera no apex do pulo, pode ser zero durante a animação de attack com lock de movimento. O `facing` só muda com input lateral mantido — resulta em câmera estável e previsível.

### 9. Deadzone: a câmera não se move com o player, mas com a borda

A deadzone de 0.5m significa que a câmera só ajusta o target quando o player sai de uma faixa central. Isso evita que a câmera vibre em resposta a micro-ajustes. O player pode fazer micro-ajustes de posição sem que a câmera se mova — comportamento padrão em Metroidvanias como Hollow Knight.

---

## Diagrama da arquitetura M3

```
GameScene::update(dt)
│
├─ m_timeScale = (hitStopTimer > 0) ? 0 : 1
├─ gdt = dt * m_timeScale
│
├─ playerControllerUpdate(gdt)   → FSM state, coyote, buffer, attack timer
│   └─ on state change → swap Animator.clip
│
├─ animationUpdate(gdt)          → advance frame, write uvMin/uvMax/flipX → SpriteRenderer
│
├─ combatPreUpdate(gdt)
│   ├─ set Hitbox.active based on attackTimer fraction
│   ├─ reposition hitbox shape along facing
│   └─ advance Health.flashTimer → SpriteRenderer.tint
│
├─ physics.step(gdt)             → Box2D (CollideContact fires here → HitEvent queue)
│
├─ physicsSyncUpdate()           → b2Body.position → Transform.position
│
├─ combatPostUpdate()
│   ├─ for each HitEvent: check Hitbox.active + hitMask
│   ├─ apply damage, invuln, flash
│   ├─ apply knockback impulse
│   ├─ requestHitStop(0.06s)    → m_hitStopTimer
│   └─ cameraAddTrauma(0.65)    → m_camState.trauma
│
└─ cameraUpdate(realDt)          → look-ahead, deadzone, trauma shake, zoom lerp
    └─ cam.setPosition / setTarget / setFov
```

---

## Bugs encontrados e corrigidos

### 1. `b2Fixture::SetEnabled()` não existe no Box2D 2.4.x

**Problema:** Tentei desabilitar o fixture de hitbox durante frames inativos com `SetEnabled(false)`. O Box2D 2.4 (versão disponível no Fedora 43 via dnf) não tem esse método — foi adicionado no Box2D 3.x.

**Solução:** Remover a chamada. O sensor fica sempre ativo no Box2D, mas o `CombatContactListener` enfileira contatos sem verificar active. O gating acontece em `combatPostUpdate` via `Hitbox.active && !hitMask`. Resultado: comportamento idêntico, zero dependência de API inexistente.

### 2. `HitEvent` const em range-loop

**Problema:** O método `events()` retornava `const std::vector<HitEvent>&`, mas `combatPostUpdate` precisava modificar campos `ev.damage` e `ev.knockbackX` dentro do loop.

**Solução:** Mudar `events()` para retornar `std::vector<HitEvent>&` (sem const). Já que o sistema é único (GameScene owna o listener), não há risco de acesso concorrente.

---

## Resultado da validação

```
[INFO] OpenGL  : 4.6 (Core Profile) Mesa 25.3.6
[INFO] Texture loaded: '…/assets/sprites/player_sheet.png' [128x128 ch=4]
[INFO] Texture loaded: '…/assets/sprites/dummy.png' [32x32 ch=4]
[INFO] M3 ready — A/D move | Space jump | J attack | F1 debug | ESC quit
[INFO] FPS: 144  |  frame: 6.96 ms
[INFO] FPS: 145  |  frame: 6.94 ms
[INFO] Shutdown clean
```

**Build:** 0 erros, 0 warnings em código próprio (warnings apenas em stb_image.h de terceiros, pré-existentes).

**M3 COMPLETO E VALIDADO.**

---

## Arquivos criados/modificados

### Engine
| Arquivo | Mudança |
|---|---|
| `engine/include/engine/render/SpriteBatch.h` | +overload UV sub-rect + flipX |
| `engine/src/render/SpriteBatch.cpp` | implementa UV params, flipX, remove hardcode |
| `engine/include/engine/render/Camera.h` | +target() getter, +setFov(), +fov() |
| `engine/src/render/Camera.cpp` | implementa setFov() |
| `engine/CMakeLists.txt` | +nlohmann_json PUBLIC, +AnimationClip.cpp |
| `engine/include/engine/animation/AnimationClip.h` | **NOVO** — struct Frame + AnimationClip |
| `engine/src/animation/AnimationClip.cpp` | **NOVO** — parser JSON via nlohmann |

### Game — Componentes
| Arquivo | Mudança |
|---|---|
| `game/src/components/SpriteRenderer.h` | +uvMin, uvMax, flipX; size semântica corrigida |
| `game/src/components/PlayerControl.h` | rewrite — PlayerState enum + timers FSM |
| `game/src/components/Animator.h` | **NOVO** — clip, time, frameIdx, finished, flipX |
| `game/src/components/Hitbox.h` | **NOVO** — fixture, damage, knockback, active, hitMask |
| `game/src/components/Hurtbox.h` | **NOVO** — fixture, owner entity |
| `game/src/components/Health.h` | **NOVO** — current, max, invulnTimer, flashTimer, dead |

### Game — Sistemas
| Arquivo | Mudança |
|---|---|
| `game/src/systems/AnimationSystem.{h,cpp}` | **NOVO** — avança frames, escreve UV/flip em SpriteRenderer |
| `game/src/systems/PlayerControllerSystem.{h,cpp}` | rewrite — FSM, coyote, buffer, variable jump |
| `game/src/systems/CombatSystem.{h,cpp}` | **NOVO** — FixtureUserData, HitEvent, contact listener, pre/post |
| `game/src/systems/CameraSystem.{h,cpp}` | **NOVO** — look-ahead, deadzone, trauma shake, zoom |
| `game/src/systems/SpriteRenderSystem.cpp` | passa uvMin/uvMax/flipX para batch.draw |

### Scene + Assets
| Arquivo | Mudança |
|---|---|
| `game/src/scenes/GameScene.{h,cpp}` | rewrite — wiring completo M3, spawn dummy, hit-stop |
| `game/CMakeLists.txt` | +3 novos .cpp ao target |
| `scripts/gen_player_sheet.py` | **NOVO** — gera atlas 128x128 + dummy 32x32 via Pillow |
| `assets/sprites/player_sheet.png` | **NOVO** — knight sprite sheet (4x4 grid) |
| `assets/sprites/dummy.png` | **NOVO** — training dummy 32x32 |
| `assets/data/player_{idle,run,jump,fall,attack}.json` | **NOVO** — clips de animação |
| `assets/data/player_attacks.json` | **NOVO** — timing e hitbox do ataque |
| `README.md` | M3 ✅, controles atualizados |

---

## Próximos passos (M4)

- Inimigos com IA FSM (Patrulha → Perseguição → Ataque → Hurt → Mort)
- Loader de mapas Tiled (.tmx) via pugixml
- Paralaxe de múltiplas camadas
- `PhysicsWorld::raycastAny()` para line-of-sight dos inimigos
- Expandir level com plataformas e seção de exploração
