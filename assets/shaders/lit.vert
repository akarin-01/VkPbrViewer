#version 450

#include "common/per_frame_data.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTangent;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec4 fragTangent;

void main()
{
    mat4 model = mat4(1.0);     // Default model
    
    fragPos = vec3(model * vec4(inPosition, 1.0));
    gl_Position = frame.viewProj * vec4(fragPos, 1.0f);

    fragNormal = mat3(transpose(inverse(model))) * inNormal;
    fragTexCoord = inTexCoord;
    fragTangent = vec4(mat3(model) * inTangent.xyz, inTangent.w);
}