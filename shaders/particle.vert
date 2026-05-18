#version 450 core

// Per-vertex (quad corner)
layout(location = 0) in vec2  aCorner;    // [-0.5,0.5] quad corners
layout(location = 1) in vec2  aCornerUV;  // [0,1] UVs

// Per-instance (divisor=1)
layout(location = 2) in vec3  iPos;
layout(location = 3) in vec4  iColor;
layout(location = 4) in float iSize;
layout(location = 5) in float iRot;

uniform mat4 uViewProj;

out vec2 vUV;
out vec4 vColor;

void main() {
    // 2D billboard: rotate corner around instance center, expand by size
    float c = cos(iRot), s = sin(iRot);
    vec2 rotCorner = vec2(c * aCorner.x - s * aCorner.y,
                          s * aCorner.x + c * aCorner.y);
    vec3 worldPos = iPos + vec3(rotCorner * iSize, 0.0);

    vUV    = aCornerUV;
    vColor = iColor;
    gl_Position = uViewProj * vec4(worldPos, 1.0);
}
