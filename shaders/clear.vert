#version 450 core

// Fullscreen triangle — no VBO, no VAO attributes.
// Positions are hardcoded; gl_VertexID selects the vertex.
// Draw with: glDrawArrays(GL_TRIANGLES, 0, 3)
//
// Vertex layout in NDC (clip space):
//   V2 (-1, 3)
//   |  \
//   |    \
//   V0----V1
// (-1,-1) (3,-1)
// The triangle extends past [0,1] UV range to cover the full screen quad.

void main() {
    const vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}
