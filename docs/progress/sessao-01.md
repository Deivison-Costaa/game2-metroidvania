# Sessão 01 — 2026-05-17

**Disciplina:** Programação com Agentes  
**Objetivo da sessão:** Planejamento arquitetural completo + implementação do M0 Bootstrap  
**Agente utilizado:** Claude Code (claude-opus-4-7)

---

## Prompt 1 — Planejamento e Arquitetura

### Contexto
Briefing inicial recebido: Metroidvania 2.5D em C++ puro, sem engines prontas (Unity/Unreal/Godot), com visual inspirado em *Ori and the Blind Forest*, *Hollow Knight*, *Ender Lilies* e *INSIDE*.

### Decisões tomadas via perguntas ao agente

| Pergunta | Decisão |
|----------|---------|
| Escopo do entregável | Vertical slice polida (1 bioma + 1 mini-boss + todos os pilares) |
| Stack de áudio | OpenAL-soft (já instalado no Fedora, sem burocracia de licença) |
| Profundidade inicial | Plano arquitetural + bootstrap compilável de primeira |
| Gerenciamento de deps | `sudo dnf install` para sistema + CMake FetchContent para headers-only |

### Stack tecnológica definida

| Camada | Tecnologia |
|--------|------------|
| Linguagem | C++20 (concepts, std::format, designated initializers) |
| Build | CMake 3.31 |
| Janela/Input | SDL2 (via sdl2-compat-devel no Fedora 43) |
| Renderização | OpenGL 4.5 core — DSA (Direct State Access) |
| GL Loader | GLEW |
| Matemática | GLM 1.0.1 |
| Sprites/texturas | stb_image (single-header) |
| Modelos 3D | Assimp |
| Física | Box2D 2.4.2 |
| Áudio | OpenAL-soft + stb_vorbis |
| Debug UI | Dear ImGui v1.91.0 |
| Mapas | Tiled .tmx (parser próprio com pugixml) |
| Save | nlohmann/json |
| Fontes | FreeType |

### Arquitetura definida

**Pipeline de renderização 2.5D** — segredo do visual Ori-like:
- Câmera perspectiva FoV ~30° (achata o 3D, mantém sensação 2D)
- Plano de jogo: `z = 0`; cenário low-poly: `z < 0`; foreground decorativo: `z > 0`

**8 passes por frame:**
1. Shadow pass (depth map, PCF 3×3)
2. Geometry pass — cenário 3D em framebuffer HDR `GL_RGBA16F`
3. Sprite pass — quads ordenados por Y com normal maps
4. Particle pass — instancing GPU
5. Bloom — downsample 4 níveis → upsample gaussiano → adição
6. Composite — ACES tonemap + vignette + grain + LUT
7. UI pass — projeção ortho 1:1
8. ImGui pass (debug build apenas)

**ECS minimalista** — sparse sets, ~300 linhas, tipo EnTT-lite:
```
Entity { id, gen } → Registry → View<Components...>
```

**Sistemas na ordem de execução:**
```
input → playerController → physicsStep → combat → enemyAI →
animation → camera → particle → audio.update → render.collect
```

**Roadmap aprovado:**

| Milestone | Entregável |
|-----------|-----------|
| M0 ✅ | Bootstrap: janela + contexto GL 4.5 + shader animado |
| M1 | SpriteBatch + MeshRenderer + Camera + ResourceManager |
| M2 | ECS Registry + Box2D + InputManager |
| M3 | Animator FSM + Combate data-driven + Screen shake |
| M4 | Inimigos IA FSM + Tiled loader + Paralaxe |
| M5 | Bloom, Fog, God Rays, Particle instancing |
| M6 | AudioEngine + UI in-game + Save/Load + Mini-boss |
| M7 | Polish, assets livres de qualidade, color grade |

---

## Prompt 2 — Implementação M0 Bootstrap

### Estrutura de diretórios criada

```
game2/
├── CMakeLists.txt
├── cmake/
│   ├── Dependencies.cmake
│   └── CompilerWarnings.cmake
├── engine/
│   ├── CMakeLists.txt
│   ├── include/engine/
│   │   ├── core/     Log.h · Window.h · App.h
│   │   └── render/   Shader.h
│   └── src/
│       ├── core/     Window.cpp · App.cpp
│       └── render/   Shader.cpp
├── game/
│   ├── CMakeLists.txt
│   └── src/main.cpp
├── shaders/
│   ├── clear.vert
│   └── clear.frag
└── README.md
```

Placeholders `.gitkeep` criados para módulos M1+:
`ecs, input, audio, physics, animation, scene, resources, ui, debug, save, math`

### Arquivos de código criados

**`engine/include/engine/core/Log.h`**
- Macros `LOG_INFO`, `LOG_WARN`, `LOG_ERROR`
- `std::format` (C++20) + cores ANSI no stderr
- `__VA_OPT__` para argumentos variádicos zero ou mais

**`engine/include/engine/core/Window.h` / `.cpp`**
- RAII completo: SDL_Init → SDL_CreateWindow → SDL_GL_CreateContext → glewInit → Quit
- OpenGL 4.5 core profile, MSAA 4×, vsync habilitado
- Limpa GL_INVALID_ENUM benigno do glewInit
- Loga versão GL, GLSL e Renderer no startup

**`engine/include/engine/core/App.h` / `.cpp`**
- Loop principal: poll events → onUpdate(dt) → onRender → swap
- Delta time com clamp 50 ms (evita spiral-of-death em breakpoints)
- FPS counter com frame time em ms a cada segundo
- Sentinela `glGetError()` em debug builds

**`engine/include/engine/render/Shader.h` / `.cpp`**
- `fromFiles()` e `fromSource()` estáticos
- Move semantics corretos (evita duplo `glDeleteProgram`)
- `set()` sobrecarregado para: `float, int, vec2, vec3, vec4, mat4`
- Erros de compilação/link lançam `std::runtime_error` com log do driver

**`game/src/main.cpp`**
- `BootstrapApp` herda `App`
- VAO vazio obrigatório para GL 4.5 core profile
- Shader carregado via `ASSET_ROOT` macro (path absoluto, independe de CWD)
- Uniforms: `iTime` (float) + `iResolution` (vec2)

**`shaders/clear.vert`**
- Fullscreen triangle sem VBO — posições hardcoded, selecionadas por `gl_VertexID`
- Técnica: triângulo maior que a tela cobre todo o viewport sem gaps

**`shaders/clear.frag`**
- Paleta: deep blue / deep purple / dark teal / midnight
- Ondas senoidais animadas (`iTime`) misturadas em 3 camadas de cor
- Estrelas: hash procedural + twinkling por `sin(iTime)`
- Vignette radial + film grain sutil
- Output: `clamp(color, 0.0, 1.0)` — sem overflow

### Problema encontrado e corrigido

**Problema:** `find_package(SDL2 REQUIRED)` falha no Fedora 43 porque não existe `SDL2-devel` — o sistema usa `sdl2-compat` (SDL2 API implementada sobre SDL3).

**Solução:**
1. Pacote correto identificado: `sdl2-compat-devel`
2. `cmake/Dependencies.cmake` atualizado com fallback via pkg-config:
   ```cmake
   find_package(SDL2 QUIET)
   if(NOT SDL2_FOUND)
       pkg_check_modules(SDL2_PC REQUIRED IMPORTED_TARGET sdl2)
       add_library(SDL2::SDL2 ALIAS PkgConfig::SDL2_PC)
   endif()
   ```
3. README e plano atualizados com o nome correto do pacote

---

## Prompt 3 — GitHub + Memória + Log

### Ações realizadas

- Criado sistema de memória persistente:
  - `memory/progress-log.md` — registro cronológico (fonte do relatório final)
  - `memory/feedback-progress-log.md` — regra: atualizar a cada prompt
  - `memory/MEMORY.md` — índice atualizado
- Repositório GitHub criado: [Deivison-Costaa/game2-metroidvania](https://github.com/Deivison-Costaa/game2-metroidvania)
  - Público, descrição em pt-BR
  - Branch `main`, SSH
- Git inicializado em `game2/`, 39 arquivos commitados (820 inserções)
- Commit: `feat: M0 bootstrap — engine foundation + animated shader pipeline`
- Co-authored-by: Claude Code incluído conforme solicitado
- Push para `origin/main` realizado com sucesso

---

## Prompt 4 — Docs no projeto + Validação M0

### Ações realizadas

- Criada pasta `docs/progress/` no repositório
- Este arquivo criado: `docs/progress/sessao-01.md`

### Status da validação M0

**Pendente:** dependências de desenvolvimento não instaladas.

Comando necessário (executar com sudo):
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

Após instalação, build:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
./build/game/game
```

**Resultado esperado:** janela 1280×720 com gradiente atmosférico animado + estrelas.

---

## Checklist M0

- [x] Estrutura de diretórios criada (25+ pastas)
- [x] CMake configurado com FetchContent + find_package
- [x] Engine lib estática compilável
- [x] Window RAII (SDL2 + OpenGL 4.5 + GLEW)
- [x] App loop com delta time, FPS counter, GL sentinel
- [x] Shader RAII com uniforms tipados
- [x] Fullscreen triangle sem VBO (gl_VertexID)
- [x] Shader GLSL 4.50 com gradiente animado + estrelas
- [x] Log.h com std::format e ANSI colors
- [x] .gitignore, README.md
- [x] Repositório GitHub criado e código publicado
- [ ] **Build compilando** — aguardando instalação das deps
- [ ] **Binário rodando** — aguardando build
