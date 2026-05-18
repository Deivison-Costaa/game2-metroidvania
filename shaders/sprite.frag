#version 450 core

in vec2  vUV;
in vec4  vColor;
in float vWorldDepth; // from sprite.vert: -aPos.z (positive = further back)

uniform sampler2D uTexture;
uniform float     uFogDensity; // 0 = no fog, ~0.3 = subtle background haze
uniform vec3      uFogColor;   // should match scene clear color

out vec4 fragColor;

void main() {
    vec4 texColor = texture(uTexture, vUV);
    vec4 col      = texColor * vColor;
    if (col.a < 0.01) discard;

    // Z-depth fog: affects only background layers (negative world Z → positive depth)
    float fog = clamp(vWorldDepth * uFogDensity * 0.2, 0.0, 1.0);
    col.rgb   = mix(col.rgb, uFogColor, fog);

    fragColor = col;
}
