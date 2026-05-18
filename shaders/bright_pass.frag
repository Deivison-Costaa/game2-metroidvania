#version 450 core

in vec2 vUV;

uniform sampler2D uHDR;
uniform float     uThreshold; // default 1.0 — extract pixels brighter than this

out vec4 fragColor;

void main() {
    vec3 col = texture(uHDR, vUV).rgb;
    // Clamp to avoid ringing artifacts in the blur
    col = min(col, vec3(20.0));
    // Luminance-weighted bright extraction
    float luma = dot(col, vec3(0.2126, 0.7152, 0.0722));
    float contribution = max(0.0, luma - uThreshold);
    // Scale the color proportionally so only the over-threshold part passes
    fragColor = vec4(col * (contribution / max(luma, 0.001)), 1.0);
}
