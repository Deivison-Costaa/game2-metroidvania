#version 450 core

// Final composite: HDR scene + bloom + god rays → LDR via ACES tonemap + gamma 2.2.
// Includes minimal 5-tap FXAA to soften edges from the non-MSAA HDR FBO.

in vec2 vUV;

uniform sampler2D uHDR;
uniform sampler2D uBloom;
uniform sampler2D uGodRays;
uniform float     uBloomStrength; // 0.04 default
uniform float     uExposure;      // 1.0 default
uniform vec2      uTexelSize;     // 1.0 / resolution (for FXAA on HDR)
uniform sampler3D uColorLUT;      // 3D LUT (Hald, N³)
uniform float     uLutStrength;   // 0 = bypass, 1 = full grade
uniform float     uLutSize;       // N (e.g. 64)

out vec4 fragColor;

// ACES filmic (Krzysztof Narkowicz, 2015 — fast approx)
vec3 aces(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Luma weight (Rec.709)
float luma(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }

// 5-tap FXAA — runs on the raw HDR texture (values ~0-4), detects edges by luma delta
vec3 fxaa(sampler2D tex, vec2 uv, vec2 texel) {
    vec3 n = texture(tex, uv + vec2( 0,  1) * texel).rgb;
    vec3 s = texture(tex, uv + vec2( 0, -1) * texel).rgb;
    vec3 e = texture(tex, uv + vec2( 1,  0) * texel).rgb;
    vec3 w = texture(tex, uv + vec2(-1,  0) * texel).rgb;
    vec3 m = texture(tex, uv).rgb;

    float lumaRange = max(luma(m), max(max(luma(n), luma(s)), max(luma(e), luma(w))))
                    - min(luma(m), min(min(luma(n), luma(s)), min(luma(e), luma(w))));
    if (lumaRange < 0.05) return m;  // below threshold — no AA needed
    return (n + s + e + w + m) / 5.0;
}

// Sample 3D LUT with half-texel offset correction (avoids clamping edge pixels)
vec3 applyLUT(vec3 c) {
    float scale  = (uLutSize - 1.0) / uLutSize;
    float offset = 0.5 / uLutSize;
    return texture(uColorLUT, clamp(c, 0.0, 1.0) * scale + offset).rgb;
}

void main() {
    vec3 scene  = fxaa(uHDR, vUV, uTexelSize);
    vec3 bloom  = texture(uBloom,   vUV).rgb;
    vec3 rays   = texture(uGodRays, vUV).rgb;

    // Additive combination in HDR space
    vec3 hdr = (scene + bloom * uBloomStrength + rays) * uExposure;

    // Tonemap + gamma
    vec3 ldr = aces(hdr);
    ldr = pow(ldr, vec3(1.0 / 2.2));

    // Color grade via 3D LUT (teal shadows, warm highlights)
    if (uLutStrength > 0.0)
        ldr = mix(ldr, applyLUT(ldr), uLutStrength);

    // Subtle radial vignette
    vec2  vc  = vUV - 0.5;
    float vig = 1.0 - dot(vc, vc) * 1.2;
    ldr *= clamp(vig, 0.0, 1.0);

    fragColor = vec4(ldr, 1.0);
}
