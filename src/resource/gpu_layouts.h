#pragma once

#include <glm/glm.hpp>

#define STD140_ASSERT(T, SIZE)\
    static_assert(sizeof(T) == SIZE, #T " std140 mismatch (expected " #SIZE")")

#define STD430_ASSERT(T, SIZE)\
    static_assert(sizeof(T) == SIZE, #T " std430 mismatch (expected " #SIZE")")

namespace Kita::Pbrv
{
    namespace Gpu
    {
        /// std140 mirrors of the UBO blocks in per_frame.glsl / per_object.glsl /
        /// post_process.frag; std430 mirrors of the compute shader push constants.

        /// Mirror of the PerFrame block in per_frame.glsl (set 0, binding 0).
        struct PerFrame
        {
            struct Camera
            {
                alignas(16) glm::mat4 m_viewProj{ 1.0f };
                alignas(16) glm::mat4 m_skyboxViewProj{ {1.0f} };
                alignas(16) glm::vec4 m_position{ 0.0f, 0.0f, 0.0f, 1.0f };         // xyz - pos, w - 1 always
            };

            struct Light
            {
                alignas(16) glm::vec4 m_position{ 0.0f, 0.0f, 0.0f, 0.0f };         // xyz - pos, w - 0(directional light)
                alignas(16) glm::vec4 m_colorIntensity{ 0.0f };                     // xyz - rgb, w - intensity
            };

            Camera m_camera{};
            Light m_light{};
        };
        STD140_ASSERT(PerFrame, 176);

        /// Mirror of the PerObject block in per_object.glsl (set 3, binding 0).
        struct PerObject
        {
            struct Transform
            {
                alignas(16) glm::mat4 m_model{ 1.0f };
                alignas(16) glm::mat4 m_normal{ 1.0f };
            };

            struct Material
            {
                alignas(16) glm::vec4 m_albedo{ 1.0f, 1.0f, 1.0f, 1.0f };
                alignas(16) glm::vec4 m_pbrParams{ 0.0f, 0.0f, 0.0f, 0.0f };   // x - metallic, y - roughness, z - ao, w - padding
                alignas(16) glm::vec4 m_emissive{ 0.0f, 0.0f, 0.0f, 1.0f };    // rgb - color, a - intensity
            };

            Transform m_transform{};
            Material m_material{};
        };
        STD140_ASSERT(PerObject, 176);

        /// Mirror of the PerPass block in post_process.frag (set 1, binding 0).
        struct PostProcess
        {
            alignas(16) glm::vec4 m_exposure;      // x - exposure, yzw - padding
        };
        STD140_ASSERT(PostProcess, 16);

        /// Push constants of irradiance_convolution.comp (std430).
        struct IrradiancePC
        {
            float m_envMip{ 0.0f };    // source cubemap sampling lod, computed from face sizes
        };
        STD430_ASSERT(IrradiancePC, 4);

        /// Push constants of prefilter.comp (std430).
        struct PrefilterPC
        {
            float m_roughness{ 0.0f };  // 0..1, selects the mip level
            float m_mipCount{ 1.0f };
        };
        STD430_ASSERT(PrefilterPC, 8);
    }
}
