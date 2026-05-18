#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3  vWorldNormal;
out vec2  vUV;
out float vWorldDepth; // -world Z (positive = further back)

void main() {
    vec3 worldPos    = vec3(uModel * vec4(aPos, 1.0));
    vWorldNormal     = normalize(mat3(transpose(inverse(uModel))) * aNormal);
    vUV              = aUV;
    vWorldDepth      = -worldPos.z;
    gl_Position      = uProj * uView * vec4(worldPos, 1.0);
}
