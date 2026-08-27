#pragma once

#include <glm/glm.hpp>

#define STD140_ASSERT(T, SIZE)\
    static_assert(sizeof(T) == SIZE, #T " std140 mismatch (expected " #SIZE")")

namespace Kita::Pbrv
{
    namespace Resource
    {
        struct FrameUbo
        {
            alignas(16) glm::mat4 m_viewProj;
            alignas(16) glm::mat4 m_skyboxViewProj;
            alignas(16) glm::vec4 m_viewPos;            // xyz - pos, w - 1 always
            alignas(16) glm::vec4 m_lightPos;           // xyz - pos, w - 0 (directional light)
            alignas(16) glm::vec4 m_lightColor;         // xyz - rgb, w - intensity
            alignas(16) glm::vec4 m_exposure;           // x - ev, yzw - padding (postprocess data)
        };
        STD140_ASSERT(FrameUbo, 192);

        struct ObjectUbo
        {
            alignas(16) glm::mat4 m_model;
            alignas(16) glm::mat4 m_normal;
            alignas(16) glm::vec4 m_albedo;
            alignas(16) glm::vec4 m_params;             // x - metallic, y - roughness, z - ao, w - padding
            alignas(16) glm::vec4 m_emissive;           // xyz - emissive, w - padding
        };
        STD140_ASSERT(ObjectUbo, 176);
    }
}
