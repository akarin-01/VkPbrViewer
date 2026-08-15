#version 450

layout(location = 0) out vec2 fragTexCoord;

void main()
{
    /* Offline triangle
     * index: 0, uv: (0, 0), pos: (-1, -1, 0)
     * index: 1, uv: (2, 0), pos: (3, -1, 0)
     * index: 2, uv: (0, 2), pos: (-1, 3, 0)
    */
    vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
    fragTexCoord = uv;
}
