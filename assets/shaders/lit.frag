#version 450

#include "common/per_frame_data.glsl"
#include "common/material_texture_slots.glsl"

#define PI 3.14159265359

layout(push_constant, std430) uniform MaterialPC
{
    vec4 albedo;
    vec4 params;            // x - metallic, y - roughness, z - ao, w - padding
    vec4 emissive;          // xyz - emissive, w - padding
} material;

layout(set = 1, binding = 0) uniform sampler2D textures[TEXTURE_COUNT];

layout(set = 2, binding = 0) uniform samplerCube irradianceMap;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec4 fragTangent;

layout(location = 0) out vec4 outColor;

// F: Schlick Fresnel
vec3 FresnelSchlick(float cosTheta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// D: GGX (Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

// G: Smith (Schlick-GGX)，k 因子区分直接光照与 IBL
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;        // Direct lighting（IBL 用 k = roughness^2 / 2）

    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 tangent = normalize(fragTangent.xyz - dot(normal, fragTangent.xyz) * normal);
    vec3 bitangent = normalize(cross(normal, tangent) * fragTangent.w);

    mat3 TBN = mat3(tangent, bitangent, normal);
    vec3 normalTS = texture(textures[NORMAL], fragTexCoord).xyz * 2.0 - 1.0;
    vec3 nDir = normalize(TBN * normalTS);

    vec3 vDir = normalize(frame.viewPos.xyz - fragPos);
    vec3 lDir = normalize(frame.lightPos.xyz);              // directional light
    vec3 hDir = normalize(vDir + lDir);

    vec4 mr = texture(textures[MR], fragTexCoord);
    vec4 albedo = material.albedo * texture(textures[ALBEDO], fragTexCoord);
    float metallic = material.params.x * mr.b;
    float roughness = material.params.y * mr.g;
    float ao = material.params.z * texture(textures[AO], fragTexCoord).r;
    vec3 emissive = material.emissive.rgb * texture(textures[EMISSIVE], fragTexCoord).rgb;
    vec3 irradiance = texture(irradianceMap, nDir).rgb;

    float NdotV = max(dot(nDir, vDir), 0.0);
    float NdotL = max(dot(nDir, lDir), 0.0);
    float HdotV = max(dot(hDir, vDir), 0.0);

    vec3 radiance = frame.lightColor.xyz * frame.lightColor.w;
    vec3 f0 = mix(vec3(0.04), albedo.rgb, metallic);

    // Cook-Torrance specular: D * F * G / (4 * NdotV * NdotL)
    vec3 f = FresnelSchlick(HdotV, f0);
    float d = DistributionGGX(nDir, hDir, roughness);
    float g = GeometrySmith(nDir, vDir, lDir, roughness);
    vec3 specular = f * d * g / max(4.0 * NdotV * NdotL, 0.001);

    // Lambert diffuse，乘 (1 - F) 保证能量守恒，metallic=1 时消失
    vec3 kd = (vec3(1.0) - f) * (1.0 - metallic);
    vec3 diffuse = kd * albedo.rgb / PI;

    // Ambient (Ibl diffuse and TODO: Ibl specular)
    vec3 ambient = kd * albedo.rgb / PI * irradiance;

    vec3 result = (diffuse + specular) * NdotL * radiance;
    result += ambient * ao;                                      
    result += emissive;
    outColor = vec4(result, albedo.a);
}
