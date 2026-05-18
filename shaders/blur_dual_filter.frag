#version 450 core

// Dual-filter blur (Marius Bjørge, SIGGRAPH 2015 Mobile).
// uMode 0 = downsample, 1 = upsample.
// Run multiple ping-pong passes for a wide smooth kernel.

in vec2 vUV;

uniform sampler2D uTex;
uniform vec2      uTexelSize; // 1.0 / textureSize
uniform int       uMode;      // 0=down, 1=up

out vec4 fragColor;

vec4 downsample(vec2 uv) {
    vec2 h = uTexelSize * 0.5;
    vec4 sum = texture(uTex, uv) * 4.0;
    sum += texture(uTex, uv + vec2(-h.x, -h.y));
    sum += texture(uTex, uv + vec2( h.x, -h.y));
    sum += texture(uTex, uv + vec2(-h.x,  h.y));
    sum += texture(uTex, uv + vec2( h.x,  h.y));
    return sum / 8.0;
}

vec4 upsample(vec2 uv) {
    vec2 h = uTexelSize;
    vec4 sum;
    sum  = texture(uTex, uv + vec2(-h.x,  0.0)) * 2.0;
    sum += texture(uTex, uv + vec2( h.x,  0.0)) * 2.0;
    sum += texture(uTex, uv + vec2( 0.0, -h.y)) * 2.0;
    sum += texture(uTex, uv + vec2( 0.0,  h.y)) * 2.0;
    sum += texture(uTex, uv + vec2(-h.x, -h.y));
    sum += texture(uTex, uv + vec2( h.x, -h.y));
    sum += texture(uTex, uv + vec2(-h.x,  h.y));
    sum += texture(uTex, uv + vec2( h.x,  h.y));
    return sum / 12.0;
}

void main() {
    fragColor = (uMode == 0) ? downsample(vUV) : upsample(vUV);
}
