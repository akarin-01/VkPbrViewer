#version 450

#include "common/constants.glsl"
#include "common/per_frame_data.glsl"
#include "common/material_texture_slots.glsl"
#include "common/ggx.glsl"

layout(push_constant, std430) uniform MaterialPC
{
    vec4 albedo;
    vec4 params;            // x - metallic, y - roughness, z - ao, w - padding
    vec4 emissive;          // xyz - emissive, w - padding
} material;

layout(set = 1, binding = 0) uniform sampler2D brdfLut;
layout(set = 2, binding = 0) uniform samplerCube iblMaps[2];        // 0 -> irradiance, 1 -> prefilter
layout(set = 3, binding = 0) uniform sampler2D textures[TEXTURE_COUNT];

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

// F: Schlick Fresnel for IBL (roughness lowers the base reflectance)
vec3 FresnelSchlickRoughness(float cosTheta, vec3 f0, float roughness)
{
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
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

    float NdotV = max(dot(nDir, vDir), 0.0);
    float NdotL = max(dot(nDir, lDir), 0.0);
    float HdotV = max(dot(hDir, vDir), 0.0);
    float NdotH = max(dot(nDir, hDir), 0.0);

    vec3 radiance = frame.lightColor.xyz * frame.lightColor.w;
    vec3 f0 = mix(vec3(0.04), albedo.rgb, metallic);

    // Cook-Torrance specular: D * F * G / (4 * NdotV * NdotL)
    vec3 f = FresnelSchlick(HdotV, f0);
    float d = DistributionGGX(NdotH, roughness);
    float g = GeometrySmith(nDir, vDir, lDir, roughness);
    vec3 specular = f * d * g / max(4.0 * NdotV * NdotL, 0.001);

    // Lambert diffuse，乘 (1 - F) 保证能量守恒，metallic=1 时消失
    vec3 kd = (vec3(1.0) - f) * (1.0 - metallic);
    vec3 diffuse = kd * albedo.rgb / PI;

    // IBL ambient: split-sum specular + irradiance diffuse
    vec3 R = reflect(-vDir, nDir);
    float mip = roughness * (textureQueryLevels(iblMaps[1]) - 1.0);
    vec3 prefilteredColor = textureLod(iblMaps[1], R, mip).rgb;
    vec2 brdf = texture(brdfLut, vec2(NdotV, roughness)).rg;

    vec3 fresnelIBL = FresnelSchlickRoughness(NdotV, f0, roughness);
    vec3 kdIBL = (vec3(1.0) - fresnelIBL) * (1.0 - metallic);
    vec3 diffuseIBL = kdIBL * albedo.rgb / PI * texture(iblMaps[0], nDir).rgb;
    vec3 specularIBL = prefilteredColor * (fresnelIBL * brdf.x + brdf.y);

    vec3 result = (diffuse + specular) * NdotL * radiance;
    result += (diffuseIBL + specularIBL) * ao;
    result += emissive;
    outColor = vec4(result, albedo.a);
}
