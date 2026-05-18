# Sessão 02 — M1 Renderer 2D + 3D

**Data:** 2026-05-17  
**Milestone:** M1 — SpriteBatch, MeshRenderer, Camera

---

## Resumo

M1 implementado e validado. Demo rodando a 144 FPS com cubo 3D (Assimp) + 5 sprites em world-space + câmera perspectiva auto-orbitando. Mesmo GL context do M0 (Mesa 4.6).

---

## Arquivos criados (engine)

| Arquivo | Descrição |
|---------|-----------|
| `engine/include/engine/render/Texture.h` | RAII GL texture, move-only, stb_image |
| `engine/src/render/Texture.cpp` | STB_IMAGE_IMPLEMENTATION aqui; fromFile + fromWhite |
| `engine/include/engine/render/Camera.h` | Perspectiva (FoV, aspect) + Ortho; view/proj lazy; setAspect para resize |
| `engine/src/render/Camera.cpp` | glm::perspective / glm::ortho / glm::lookAt |
| `engine/include/engine/render/SpriteBatch.h` | Quad batcher 1024 sprites, begin/draw/end, flush on texture switch |
| `engine/src/render/SpriteBatch.cpp` | VAO+VBO(stream)+IBO(static), 36-byte vertex layout, orphan-and-fill |
| `engine/include/engine/render/Mesh.h` | VBO/EBO/VAO RAII, move-only |
| `engine/src/render/Mesh.cpp` | Assimp loader: fromFile, tri+normals+flip UV |
| `engine/include/engine/render/MeshRenderer.h` | Desenha Mesh com camera + modelo + luz Lambert |
| `engine/src/render/MeshRenderer.cpp` | bind shader, set uniforms, mesh.draw() |
| `engine/include/engine/resources/ResourceManager.h` | Template header-only cache shared_ptr<T> |

## Arquivos criados (shaders)

| Arquivo | Descrição |
|---------|-----------|
| `shaders/sprite.vert` | World-space quad; in vec3 aPos, vec2 aUV, vec4 aColor; uniform uViewProj |
| `shaders/sprite.frag` | texture * tint; discard alpha < 0.01 |
| `shaders/mesh.vert` | pos/normal/uv; uModel/uView/uProj; passa worldNormal |
| `shaders/mesh.frag` | Lambert: albedo * (ambient + lightColor * diffuse) |

## Arquivos criados (assets de teste)

| Arquivo | Descrição |
|---------|-----------|
| `assets/sprites/test.png` | Silhueta de cavaleiro 32×32 RGBA gerada via Python |
| `assets/models/cube.obj` | Cubo 1×1×1 OBJ ASCII (24 verts, 12 tris, UVs + normais por face) |

## Arquivos modificados

| Arquivo | Mudança |
|---------|---------|
| `engine/CMakeLists.txt` | Adicionados 5 novos .cpp; link `stb PRIVATE`, `assimp::assimp PRIVATE` |
| `cmake/Dependencies.cmake` | assimp QUIET → REQUIRED |
| `engine/include/engine/core/Window.h` | `setSize(w, h)` adicionado para resize handling |
| `engine/src/core/App.cpp` | `SDL_WINDOWEVENT_SIZE_CHANGED` → `glViewport` + `setSize` |
| `game/CMakeLists.txt` | POST_BUILD copia `assets/` junto com `shaders/` |
| `game/src/main.cpp` | BootstrapApp substituído por `GameApp` (M1 demo) |
| `.gitignore` | Exceções `!assets/sprites/test.png` e `!assets/models/cube.obj` |
| `README.md` | M1 marcado ✅ |

---

## Resultado da validação

```
[INFO] OpenGL  : 4.6 (Core Profile) Mesa 25.3.6
[INFO] GLSL    : 4.60
[INFO] Renderer: Mesa Intel(R) Graphics (RPL-S)
[INFO] Texture loaded: '…/assets/sprites/test.png' [32x32 ch=4]
[INFO] Mesh loaded: '…/assets/models/cube.obj' [24 verts, 12 tris]
[INFO] M1 ready — ESC to quit, resize window to test Camera
[INFO] FPS: 144  |  frame: 6.99 ms
[INFO] Shutdown clean
```

**M1 COMPLETO E VALIDADO.**

---

## Decisões técnicas

- **Mesh::fromFile via Assimp** — mesma interface `fromFile` que Texture, compatível com ResourceManager
- **SpriteBatch::flush usa orphan-and-fill** — `glBufferData(nullptr) + glBufferSubData` para evitar stall de GPU
- **Depth mask OFF em sprites** — `glDepthMask(GL_FALSE)` no flush, restaurado após. Sprites ocluem com o cubo mas não com si mesmos
- **Camera lazy view** — dirty flag evita recomputar `lookAt` todo frame quando não há mudança
- **stb warnings isolados via `#pragma GCC diagnostic`** — não foi possível suprimir todos, mas warnings estão no código de terceiro (stb_image.h)

---

## Próximos passos (M2)

- ECS: Registry sparse-set (~300 linhas)
- Box2D: integração física, corpos dinâmicos
- Input: sistema de mapeamento de teclas (action map)
- Componentes iniciais: Transform, Rigidbody, SpriteRenderer, MeshRenderer
- Player proto: movimento horizontal, salto, colisão com chão
