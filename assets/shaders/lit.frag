#version 450

#include "include/constants.glsl"
#include "include/per_frame.glsl"
#include "include/per_material.glsl"
#include "include/per_object.glsl"
#include "include/ggx.glsl"

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

// G: Smith (Schlick-GGX); the k factor differs between direct lighting and IBL
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
    vec3 normalTS = texture(texNormal, fragTexCoord).xyz * 2.0 - 1.0;
    vec3 nDir = normalize(TBN * normalTS);

    vec3 vDir = normalize(frame.camera.position.xyz - fragPos);
    vec3 lDir = normalize(frame.light.position.xyz);              // directional light
    vec3 hDir = normalize(vDir + lDir);

    vec4 mr = texture(texMetalRoughness, fragTexCoord);
    vec4 albedo = object.material.albedo * texture(texAlbedo, fragTexCoord);
    float metallic = object.material.pbrParams.x * mr.b;
    float roughness = object.material.pbrParams.y * mr.g;
    float ao = object.material.pbrParams.z * texture(texAo, fragTexCoord).r;
    vec3 emissive = object.material.emissive.rgb * object.material.emissive.a * texture(texEmissive, fragTexCoord).rgb;

    float NdotV = max(dot(nDir, vDir), 0.0);
    float NdotL = max(dot(nDir, lDir), 0.0);
    float HdotV = max(dot(hDir, vDir), 0.0);
    float NdotH = max(dot(nDir, hDir), 0.0);

    vec3 radiance = frame.light.colorIntensity.rgb * frame.light.colorIntensity.a;
    vec3 f0 = mix(vec3(0.04), albedo.rgb, metallic);

    // Cook-Torrance specular: D * F * G / (4 * NdotV * NdotL)
    vec3 f = FresnelSchlick(HdotV, f0);
    float d = DistributionGGX(NdotH, roughness);
    float g = GeometrySmith(nDir, vDir, lDir, roughness);
    vec3 specular = f * d * g / max(4.0 * NdotV * NdotL, 0.001);

    // Lambert diffuse; (1 - F) for energy conservation, zero at metallic = 1
    vec3 kd = (vec3(1.0) - f) * (1.0 - metallic);
    vec3 diffuse = kd * albedo.rgb / PI;

    // IBL ambient: split-sum specular + irradiance diffuse
    vec3 R = reflect(-vDir, nDir);
    float mip = roughness * (textureQueryLevels(texPrefilter) - 1.0);
    vec3 prefilteredColor = textureLod(texPrefilter, R, mip).rgb;
    vec2 brdf = texture(texBrdfLut, vec2(NdotV, roughness)).rg;
    vec3 fresnelIBL = FresnelSchlickRoughness(NdotV, f0, roughness);
    vec3 specularIBL = prefilteredColor * (fresnelIBL * brdf.x + brdf.y);

    vec3 kdIBL = (vec3(1.0) - fresnelIBL) * (1.0 - metallic);
    vec3 diffuseIBL = kdIBL * albedo.rgb / PI * texture(texIrradiance, nDir).rgb;

    vec3 result = (diffuse + specular) * NdotL * radiance;
    result += (diffuseIBL + specularIBL) * ao;
    result += emissive;
    outColor = vec4(result, albedo.a);
}
