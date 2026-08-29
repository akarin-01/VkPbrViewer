#version 450

#include "include/per_frame.glsl"

layout(location = 0) in vec3 fragDir;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 color = texture(texSkybox, fragDir).rgb;
    outColor = vec4(color, 1.0);
}
