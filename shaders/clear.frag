#version 450 core

uniform float iTime;
uniform vec2  iResolution;

out vec4 fragColor;

// ── Utility ───────────────────────────────────────────────────────────────────

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// ── Main ──────────────────────────────────────────────────────────────────────

void main() {
    vec2 uv = gl_FragCoord.xy / iResolution; // [0,1]

    // Palette — dark atmospheric colors matching the game's visual target
    vec3 deepBlue    = vec3(0.04, 0.06, 0.22);
    vec3 deepPurple  = vec3(0.18, 0.04, 0.30);
    vec3 darkTeal    = vec3(0.02, 0.14, 0.20);
    vec3 midnight    = vec3(0.01, 0.01, 0.05);

    // Animated waves
    float wave1 = sin(uv.x * 5.0 + iTime * 0.7) * 0.08;
    float wave2 = sin(uv.y * 3.0 + iTime * 0.5) * 0.06;
    float wave3 = sin((uv.x + uv.y) * 4.0 - iTime * 0.4) * 0.05;

    float t = sin(iTime * 0.3) * 0.5 + 0.5;

    // Layer the colors
    vec3 color = mix(deepBlue, deepPurple, uv.x + wave1);
    color      = mix(color, darkTeal,     uv.y * 0.6 + wave2 + t * 0.2);
    color      = mix(color, midnight,     (1.0 - uv.y) * 0.3 + wave3);

    // Subtle "stars" — random bright pixels
    float star = step(0.998, hash(floor(gl_FragCoord.xy / 2.0) + floor(iTime * 0.1)));
    float twinkle = sin(iTime * 3.0 + hash(floor(gl_FragCoord.xy / 2.0)) * 6.28) * 0.5 + 0.5;
    color += vec3(star * twinkle * 0.8);

    // Vignette
    vec2  centered = uv - 0.5;
    float vignette = 1.0 - dot(centered, centered) * 1.8;
    color *= clamp(vignette, 0.0, 1.0);

    // Subtle film grain
    float grain = (hash(uv + fract(iTime)) - 0.5) * 0.03;
    color += vec3(grain);

    fragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
