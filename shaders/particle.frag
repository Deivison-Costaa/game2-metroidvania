#version 450 core

in vec2 vUV;
in vec4 vColor;

uniform sampler2D uTex; // radial gradient sprite (alpha channel)

out vec4 fragColor;

void main() {
    float mask  = texture(uTex, vUV).a;
    float alpha = mask * vColor.a;
    if (alpha < 0.01) discard;
    // vColor.rgb can be > 1.0 to feed the HDR bloom pass
    fragColor = vec4(vColor.rgb, alpha);
}
