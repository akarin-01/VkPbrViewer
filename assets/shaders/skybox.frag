#version 450

layout(set = 1, binding = 0) uniform samplerCube skybox;

layout(location = 0) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(0.30, 0.45, 0.60, 1.0);
}