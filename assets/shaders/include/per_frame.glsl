#ifndef PER_FRAME_GLSL
#define PER_FRAME_GLSL

#include "include/shader_sets.glsl"

struct PerFrameCamera
{
    mat4 viewProj;
    mat4 skyboxViewProj;
    vec4 position;           // xyz - pos, w - 1 always
};

struct PerFrameLight
{
    vec4 position;          // xyz - pos, w - 0(directional light)
    vec4 colorIntensity;    // xyz - color, w - intensity
};

layout(set = SET_PER_FRAME, binding = 0, std140) uniform PerFrame
{
    PerFrameCamera camera;
    PerFrameLight light;
} frame;

layout(set = SET_PER_FRAME, binding = 1) uniform sampler2D texBrdfLut;
layout(set = SET_PER_FRAME, binding = 2) uniform samplerCube texSkybox;
layout(set = SET_PER_FRAME, binding = 3) uniform samplerCube texIrradiance;
layout(set = SET_PER_FRAME, binding = 4) uniform samplerCube texPrefilter;

#endif
