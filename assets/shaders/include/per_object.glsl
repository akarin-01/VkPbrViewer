#ifndef PER_OBJECT_GLSL
#define PER_OBJECT_GLSL

#include "include/shader_sets.glsl"

struct PerObjectTransform
{
    mat4 modelMat;
    mat4 normalMat;
};

struct PerObjectMaterial
{
    vec4 albedo;
    vec4 pbrParams;         // x - metallic, y - roughness, z - ao, w - padding
    vec4 emissive;          // rgb - color, a - intensity
};

layout(set = SET_PER_OBJECT, binding = 0, std140) uniform PerObject
{
    PerObjectTransform transform;
    PerObjectMaterial material;
} object;

#endif
