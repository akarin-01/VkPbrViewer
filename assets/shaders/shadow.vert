#version 450

#include "include/per_frame.glsl"
#include "include/per_object.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTangent;

void main()
{
    gl_Position = frame.light.lightSpace * object.transform.modelMat * vec4(inPosition, 1.0);
}
