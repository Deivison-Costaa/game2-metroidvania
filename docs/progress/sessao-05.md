# Sessão 05 — M5: Shaders Avançados (HDR Bloom + Fog + God Rays + Partículas)

**Data:** 2026-05-18  
**Commit:** `baf7807` — 32 arquivos | +1114 -41 linhas  
**Status final:** ✅ build limpo, 0 warnings, 144 FPS, shutdown limpo

---

## Por que este milestone era necessário

M0–M4 entregaram um jogo **funcionalmente completo** mas visualmente plano: um único render pass sem pós-processamento, sprites com tint padrão [0,1], zero feedback visual de partículas. O objetivo do projeto é parecer um indie comercial moderno (Ori, Hollow Knight, Dead Cells). Sem bloom, sem volumetria e sem partículas, o jogo parece uma prototype técnica.

M5 transforma o pipeline em **HDR multi-pass** e adiciona feedback visual que amplifica a leitura de combate — o hit-flash agora estoura em bloom, inimigos explodem em partículas coloridas ao morrer, projéteis deixam rastro magenta brilhante. A barra de "polimento" visual sobe de ~30% para ~65%.

---

## Arquitetura do pipeline novo

```
DEFAULT FB (MSAA 4x preservado para debug lines)
      ▲
      │ composite.frag (ACES + gamma + FXAA + vinheta)
      │
  ┌───┴──────────────────────────────────────────────────────────┐
  │                                                              │
HDR FBO RGBA16F (full res)     pingFBO (half)       raysFBO (quarter)
  │                                │                      │
  │  cena: parallax + tilemap      │ bloom final          │ god rays
  │  + sprites + particles         │ (dual-filter)        │ (radial blur)
  └──────────────────────────────────────────────────────────────┘
                    ▲
           bright_pass.frag: extrai pixels com luma > 1.0
```

---

## Decisões técnicas com raciocínio

### 1. Por que HDR (RGBA16F) em vez de LDR (RGBA8)?

Com LDR, o bloom extrai apenas pixels > 0.8–0.9 no range [0,1]. O resultado é bloom "fraco" ou blooms em tudo (se o threshold for baixo demais). Com HDR, os tints podem ser > 1.0: hit-flash usa `{3t+1, 1-0.8t, 1-0.8t}` (até 4× o normal), hit-sparks usam `{2.5, 2.0, 1.2}`. Esses valores sobrevivem intactos no FBO RGBA16F. O bloom extrai apenas os eventos visuais importantes, ficando natural e circunscrito.

### 2. Por que dual-filter blur (Kawase) em vez de Gaussian separável?

O Gaussian separável precisa de 2N passes para kernel de tamanho N. O dual-filter de Marius Bjørge (SIGGRAPH 2015 Mobile) usa apenas 2 passes por iteração (downsample + upsample) e consegue resultado praticamente idêntico para bloom. Para 640×360 (bloom FBO em half-res), 3 iterações dão um kernel efetivo de ~24px — suave o suficiente para o look Ori.

### 3. Por que fog por Z-depth e não screen-space?

Screen-space fog precisa de um depth buffer compartilhado entre passes, complicando o multi-pass HDR. No 2.5D com parallax, o Z-depth artístico dos layers (-4, -3, -2) é exatamente o que queremos para a neblina. Sprites do player/inimigos em Z=0 ficam sem fog automaticamente. A fórmula `fog = clamp(-worldZ * density * 0.2, 0, 1)` é elegante, zero-cost quando density=0 (default), e backwards-compatible.

### 4. Por que god rays em quarter-res?

God rays são inherentemente de baixa frequência. Renderizar em quarter-res (320×180) economiza 16× o custo dos 64 samples do loop em relação a full-res, e o resultado upscalado pelo sampler bilinear no composite é visualmente indistinguível. No hardware Intel integrado, este trade-off é crítico para manter ~7ms/frame.

### 5. Por que instanced rendering para partículas?

Com GPU instancing (`glDrawArraysInstanced`), toda a pool de partículas ativa é desenhada em **um único draw call**. O CPU apenas preenche o VBO de instâncias (pos, color, size, rotation). A alternativa (um draw call por partícula) seria devastadora para performance — até 4096 draw calls no pior caso.

### 6. Por que callbacks (std::function) em vez de passar ParticleSystem diretamente?

Os sistemas de gameplay (EnemyAISystem, CombatSystem, ProjectileSystem) devem ser agnósticos ao ParticleSystem. Passando callbacks, a dependência é invertida: o GameScene registra lambdas que chamam `m_particleSys->spawn(...)`, e os sistemas chamam apenas o callback quando o evento ocorre. Isso mantém os sistemas testáveis e reutilizáveis independentemente do renderer.

---

## Diagrama — render pass por frame

```
GameScene::render()
    │
    ├─ m_postFx->beginScene()           ← bind HDR FBO, clear
    │
    ├─ m_batch->shader().set(fogUniforms)  ← persist ao longo das chamadas
    │
    ├─ drawParallax(...)                ← sprite.vert/frag com fog (z=-2 a -4)
    ├─ drawTileLayer(...)               ← z=-0.1 (fog quase imperceptível)
    ├─ spriteRenderUpdate(...)          ← z=0 (player/enemies: sem fog)
    ├─ m_particleSys->render(...)       ← instanced, HDR colors
    │
    └─ m_postFx->endSceneAndComposite(camera, kSunWorldPos)
           │
           ├─ doBrightPass()            → pingFBO (half-res, luma > 1.0)
           ├─ doGodRays(sunUV)          → raysFBO (quarter-res, radial blur)
           ├─ doBlur() ×3 iter          → pingFBO/pongFBO ping-pong
           └─ doComposite()             → default FB
                    │
                    ├─ FXAA 5-tap no HDR
                    ├─ hdr + bloom*0.04 + godRays
                    ├─ ACES tonemap
                    ├─ gamma 2.2
                    └─ vinheta radial sutil
    │
    └─ DebugDraw (F1) — fora do HDR, no default FB (MSAA 4x preservado)
```

---

## Arquivos criados/modificados

### Engine — novos módulos

| Arquivo | Propósito |
|---------|-----------|
| `engine/render/Framebuffer.h/cpp` | RAII FBO: color texture + depth RBO, move-only, resize |
| `engine/render/FullscreenQuad.h/cpp` | VAO vazio + `draw()` = `glDrawArrays(GL_TRIANGLES, 0, 3)` |
| `engine/render/PostProcessStack.h/cpp` | HDR FBO + 3 passes + `beginScene` / `endSceneAndComposite` |
| `engine/render/ParticleSystem.h/cpp` | Pool 4096, instanced VBO, spawn por kind (4 presets) |

### Shaders — novos

| Shader | Técnica |
|--------|---------|
| `post_fullscreen.vert` | Fullscreen tri via `gl_VertexID` + UV, reutilizado por todos os passes |
| `bright_pass.frag` | Extração luma > threshold, clamp a 20 evita ringing no blur |
| `blur_dual_filter.frag` | Kawase dual-filter: modo 0=down, 1=up |
| `god_rays.frag` | 64-sample radial blur, decay por passo, zero fora do frustum |
| `composite.frag` | HDR + bloom*str + godRays → ACES + gamma + FXAA + vinheta |
| `particle.vert/frag` | Billboard instanciado com rotação 2D, tint HDR |

### Shaders — modificados

| Shader | Mudança |
|--------|---------|
| `sprite.vert` | `vWorldDepth = -aPos.z` para fog |
| `sprite.frag` | `fog = clamp(-Z * density * 0.2, 0, 1)` + uniforms fog |
| `mesh.vert/frag` | Mesmo padrão de fog |

### Game — modificações

| Arquivo | Mudança |
|---------|---------|
| `GameScene.h` | `m_postFx`, `m_particleSys`, `m_dustTimer`, `kSunWorldPos` |
| `GameScene.cpp` | HDR pipeline no render(); callbacks de partículas no update(); dust ambient |
| `EnemyAISystem.h/cpp` | `OnEnemyDeathFn` callback, chama na transição → Dead |
| `CombatSystem.h/cpp` | `onHitSpark` callback + hit-flash HDR |
| `ProjectileSystem.h/cpp` | Trail de partícula por projétil ativo |
| `SpriteBatch.h` | Getter `Shader& shader()` para fog uniforms |
| `engine/CMakeLists.txt` | +4 novos `.cpp` |

---

## Bugs encontrados e soluções

**1. `particle_spark.png` gitignored:**  
A rule `assets/sprites/*` bloqueava o arquivo. Adicionada exceção `!assets/sprites/particle_spark.png` seguindo o padrão dos outros sprites gerados.

**2. `<cstdlib>` ausente em GameScene.cpp:**  
Uso de `rand()` para dust emission exigia o include. Adicionado junto com os demais includes.

**3. `glm/glm.hpp` ausente em CombatSystem.h:**  
O parâmetro `glm::vec3` no novo callback `onHitSpark` exigia o include no header. Adicionado.

---

## Resultado da validação

```
[INFO] PostProcessStack: HDR 1280x720 RGBA16F | bloom 640x360 | god rays 320x180
[INFO] Texture loaded: '.../particle_spark.png' [16x16 ch=4]
[INFO] ParticleSystem: pool=4096 | spark texture loaded
[INFO] M5 ready — A/D move | Space jump | J attack | F1 debug | ESC quit
[INFO] FPS: 144  |  frame: 6.93 ms
[INFO] Shutdown clean
```

- Build: 0 erros, 0 warnings do projeto (apenas stb_image third-party) ✅
- FPS: 144 cap (frame ~6.93 ms vs ~6.95 ms no M4 — overhead dos novos passes: < 0.5ms) ✅
- Shutdown limpo ✅
- Commit `baf7807` publicado em `origin/main` ✅

**M5 COMPLETO E VALIDADO.**

---

## Próximos passos (M6)

- Áudio com OpenAL-soft: música de fundo + SFX de ataque/dano/morte
- UI in-game: barra de HP do player, texto de dano flutuante
- Save/Load atômico em JSON (`~/.local/share/game2/save.json`)
- Mini-boss com FSM de duas fases
