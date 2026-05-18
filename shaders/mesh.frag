#version 450 core

in vec3  vWorldNormal;
in vec2  vUV;
in float vWorldDepth;

uniform sampler2D uTexture;
uniform vec3      uLightDir;
uniform vec3      uLightColor;
uniform vec3      uAmbient;
uniform float     uFogDensity;
uniform vec3      uFogColor;

out vec4 fragColor;

void main() {
    vec4 albedo = texture(uTexture, vUV);

    float diff  = max(dot(normalize(vWorldNormal), normalize(uLightDir)), 0.0);
    vec3  color = albedo.rgb * (uAmbient + uLightColor * diff);

    float fog = clamp(vWorldDepth * uFogDensity * 0.2, 0.0, 1.0);
    color     = mix(color, uFogColor, fog);

    fragColor = vec4(color, albedo.a);
}
