#version 450

#include "common/per_frame_data.glsl"

layout(std140, set = 1, binding = 0) uniform MaterialUbo
{
    vec4 albedo;
    vec4 params;            // x - metallic, y - roughness, z - ao, w - padding
} material;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 nDir = normalize(fragNormal);
    vec3 vDir = normalize(frame.viewPos.xyz - fragPos);
    vec3 lDir = normalize(frame.lightDir.xyz);
    vec3 hDir = normalize(vDir + lDir);

    vec3 lightColor = frame.lightColor.xyz;
    float intensity = frame.lightColor.w;
    float metallic = material.params.x;
    float roughness = material.params.y;
    float ao = material.params.z;
    vec3 albedo = material.albedo.xyz;
    float alpha = material.albedo.w;

    // Lambert diffuse (metallic=1 → no diffuse)
    float diff = max(dot(nDir, lDir), 0.0);
    vec3 diffuse = diff * lightColor * intensity * albedo * (1.0 - metallic);

    // Blinn-Phong specular (f0: metallic=0 → 0.04, metallic=1 → albedo)
    float shininess = exp2((1.0 - roughness) * 10.0);
    float spec = pow(max(dot(nDir, hDir), 0.0), shininess);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 specular = spec * lightColor * intensity * f0;

    // Ambient (temporary — replaced by IBL in the future)
    vec3 ambient = vec3(0.03) * albedo * ao * (1.0 - metallic);

    vec3 result = diffuse + specular + ambient;
    outColor = vec4(result, alpha);
}