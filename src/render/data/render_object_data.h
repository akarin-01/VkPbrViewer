#pragma once

#include "core/macro.h"
#include "rhi/constants.h"
#include "resource/types.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>

namespace Kita::Pbrv
{
    namespace Scene
    {
        class Object;
    }
    namespace Rhi
    {
        class Context;
    }
    namespace Resource
    {
        class Resources;
        class DescriptorManager;
    }

    namespace Render
    {
        /// std140 ABI mirror of the per-object block in per_object.glsl.
        struct ObjectUbo
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
        STD140_ASSERT(ObjectUbo, 176);

        /// Set 3 — per-object UBO (transform + material params), one instance
        /// per scene object. Exclusive state: no sharing, dies with the object.
        class RenderObjectData
        {
        public:
            RenderObjectData(const Rhi::Context& context,
                Resource::Resources& resources,
                Resource::DescriptorManager& descriptorMgr);
            ~RenderObjectData();

            void UpdateUbo(uint32_t frameIndex, const Scene::Object& object);

            VkDescriptorSetLayout GetSetLayout() const;
            const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]; }

        private:
            void WriteSet(uint32_t frameIndex) const;

        private:
            const Rhi::Context& m_context;
            Resource::Resources& m_resources;
            Resource::DescriptorManager& m_descriptorMgr;

            std::array<Resource::RenderBufferHandle, Rhi::kMaxFramesInFlight> m_uboHandles{};
            std::array<VkDescriptorSet, Rhi::kMaxFramesInFlight> m_sets{};
        };
    }
}
