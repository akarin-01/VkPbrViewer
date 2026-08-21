#ifndef PER_FRAME_DATA
#define PER_FRAME_DATA

layout(std140, set = 0, binding = 0) uniform PerFrame
{
    mat4 viewProj;
    mat4 skyboxViewProj;
    vec4 viewPos;           // xyz - pos, w - 1 always
    vec4 lightPos;          // xyz - pos, w - 0(directional light)
    vec4 lightColor;        // xyz - rgb, w - intensity
} frame;

#endif
