#version 450 core

in vec2 vUV;
in vec4 vColor;

uniform sampler2D uTexture;

out vec4 fragColor;

void main() {
    vec4 texColor = texture(uTexture, vUV);
    fragColor = texColor * vColor;
    if (fragColor.a < 0.01)
        discard;
}
