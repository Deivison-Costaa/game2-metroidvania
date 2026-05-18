# Sessão 06 — M6: Áudio + HUD + Save/Load + Mini-boss

**Data:** 2026-05-18  
**Milestone:** M6

## Contexto

M5 fechou o pipeline visual (HDR bloom, fog volumétrico, god rays, partículas). M6 entrega o que faltava para a vertical slice ser jogável de ponta a ponta: feedback sonoro, HUD in-game com informação de HP, persistência de estado e um mini-boss de 3 fases.

## O que foi feito

### Fase A — Áudio (OpenAL-soft)

**Arquitetura:**
- `AudioEngine` singleton com pool de 8 `AudioSource` para SFX + source dedicada para música
- `AudioBuffer` RAII wrapper (move-only), parser RIFF inline mono/stereo 16-bit PCM
- `AudioSource` RAII (`alGenSources`/`alDeleteSources`), suporte a `setLooping`, `setGain`, `play`/`stop`

**Assets procedurais (stdlib Python):**
- `gen_sfx.py` — 6 WAVs via ADSR + síntese senoidal/ruído: `attack.wav`, `hit.wav`, `jump.wav`, `land.wav`, `death.wav`, `pickup.wav`
- `gen_music.py` — drone pad estéreo, 4 vozes (A1/E2/A2/E3 com vibrato), cross-fade 10% para loop seamless

**Hooks de SFX:**
- `playerControllerUpdate` aceita `onSfx` callback — dispara `jump`, `attack`, `land`
- `combatPostUpdate` aceita `onDamageDealt` callback — dispara `hit` + HUD popup

### Fase B — Font + TextRenderer

- `Font::loadFromTTF(path, px)` — Freetype bakes glyphs ASCII 32-126 em atlas 512×512 RGBA
- `Texture::fromRawOwned(glId, w, h)` adicionado ao engine para adoção de GL IDs externos
- `TextRenderer::drawText(batch, font, text, pos, color, scale)` — quads via SpriteBatch

### Fase C — HUD

- Camera ortográfica UI separada (0..W, H..0, origin top-left)
- Barra de HP do player (cor verde→amarelo→vermelho por fração)
- Barra de boss (50% largura, centralizada no topo; cor cinza/laranja/vermelho por fase)
- Flash branco de 0.5s na transição de fase do boss
- Damage popups: projeção world→screen, rise 40px, fade 0.8s

### Fase D — Save/Load

- `SaveData`: version, player{pos, hp}, defeated[], playtime, settings{volumes}
- Save atômico: escreve `.tmp` → `fs::rename` — nunca deixa arquivo corrompido
- Caminho XDG: `$XDG_DATA_HOME/game2/save.json` com fallback `~/.local/share/`
- F5 = salvar manual, F9 = carregar; autosave quando boss morre

### Fase E — Mini-boss 3 Fases

**Componente:** `BossPhaseData` separado de `EnemyAI` (EnemyKind::MiniBoss adicionado)

**FSM:** Stance (HP 100%→66%) → Enrage (66%→33%) → Desperate (33%→0%)
- Transição: freeze 0.3s + flash HUD + camera trauma 0.7 + hit-stop 0.15s + SFX
- Fase Stance: slash melee com windup/active/recovery
- Fase Enrage: dash + slash (velocidade ×1.5)
- Fase Desperate: fogo radial 8 direções via `spawnProjectile`

**Data-driven:** `boss_attacks.json` (3 fases × janelas de ataque com hitbox, dashVel, radial config)

**Sprite:** `gen_boss_sheet.py` — sheet 256×512 (4 cols × 8 rows × 64px)

## Decisões técnicas

| Decisão | Alternativa descartada | Motivo |
|---------|----------------------|--------|
| HUD via SpriteBatch + Freetype | Dear ImGui | Visual "indie comercial" vs debug look |
| Boss em sistema separado `BossLogic` | Estender EnemyAISystem | Evita complexidade no sistema genérico |
| SFX via callbacks `std::function` | Acoplamento direto ao AudioEngine | Mantém sistemas de jogo livres de deps de áudio |
| Save atômico (tmp+rename) | fwrite direto | Nunca corrompido mesmo com crash mid-write |
| Clips de animação do boss data-driven | Hardcoded no código | Mesma infra que inimigos normais |

## Bugs encontrados e resolvidos

1. **AudioBuffer constructor privado** — free function `loadWav()` não acessava. Fix: `static AudioBuffer adopt(ALuint id)` factory pública.
2. **`Texture::fromRaw` inexistente** — Font tentava adotar GL ID sem API. Fix: adicionado `fromRawOwned(glId, w, h)` ao engine.
3. **`Hud::Camera` default-constructor privado** — `Camera()` é private, Hud não compilava. Fix: initializer inline com `Camera::orthographic(...)`.
4. **`LOG_INFO` com string concatenada** — macro usa `std::format` que requer constexpr format string. Fix: usar args do format (`LOG_INFO("msg {}", val)`).
5. **`static_cast<uint32_t>(Entity)`** — Entity é struct, não tem conversão implícita. Fix: usar `en.id`.

## Validação

```
[INFO] M6 ready — A/D move | Space jump | J attack | F1 debug | F5 save | F9 load | ESC quit
```

Build limpo: `[100%] Built target game` — 0 erros, 0 warnings.

## Próximo — M7

Polish + assets finais + color grade:
- Substituir sprites placeholder por arte final
- Tela de game-over quando player morre
- Menu principal / tela de título
- Color grading LUT
- Polimento de animações (transições suaves)
