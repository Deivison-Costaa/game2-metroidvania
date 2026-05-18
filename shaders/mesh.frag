#version 450 core

in vec3 vWorldNormal;
in vec2 vUV;

uniform sampler2D uTexture;
uniform vec3      uLightDir;   // normalized, world-space
uniform vec3      uLightColor;
uniform vec3      uAmbient;

out vec4 fragColor;

void main() {
    vec4 albedo = texture(uTexture, vUV);

    float diff   = max(dot(normalize(vWorldNormal), normalize(uLightDir)), 0.0);
    vec3  color  = albedo.rgb * (uAmbient + uLightColor * diff);

    fragColor = vec4(color, albedo.a);
}
