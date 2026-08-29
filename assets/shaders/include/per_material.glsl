#ifndef PER_MATERIAL_GLSL
#define PER_MATERIAL_GLSL

#include "include/shader_sets.glsl"

layout(set = SET_PER_MATERIAL, binding = 0) uniform sampler2D texAlbedo;
layout(set = SET_PER_MATERIAL, binding = 1) uniform sampler2D texNormal;
layout(set = SET_PER_MATERIAL, binding = 2) uniform sampler2D texMetalRoughness;
layout(set = SET_PER_MATERIAL, binding = 3) uniform sampler2D texAo;
layout(set = SET_PER_MATERIAL, binding = 4) uniform sampler2D texEmissive;

#endif
