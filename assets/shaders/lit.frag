#version 450

layout(location = 0) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 result = fragNormal * 0.5 + 0.5;
    outColor = vec4(result, 1.0);
}