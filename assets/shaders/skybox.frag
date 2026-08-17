#version 450

layout(set = 1, binding = 0) uniform samplerCube skybox;

layout(location = 0) in vec3 fragDir;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 color = texture(skybox, fragDir).rgb;
    outColor = vec4(color, 1.0);
}
