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
        class Camera;
        class Light;
    }
    namespace Rhi
    {
        class RenderContext;
        class SwapChain;
    }
    namespace Resource
    {
        class RenderResources;
        class DescriptorManager;
    }

    namespace Render
    {
        struct FrameUbo
        {
            alignas(16) glm::mat4 m_viewProj;
            alignas(16) glm::mat4 m_skyboxViewProj;
            alignas(16) glm::vec4 m_viewPos;            // xyz - pos, w - 1 always
            alignas(16) glm::vec4 m_lightPos;           // xyz - pos, w - 0(directional light)
            alignas(16) glm::vec4 m_lightColor;         // xyz - rgb, w - intensity
        };
        STD140_ASSERT(FrameUbo, 176);

        class RenderFrameData
        {
        public:
            RenderFrameData(const Rhi::RenderContext& context,
                Resource::RenderResources& resources,
                const Rhi::SwapChain& swapChain,
                Resource::DescriptorManager& descriptorMgr);
            ~RenderFrameData();

            void Update(uint32_t frameIndex, const Scene::Camera& camera, const Scene::Light& light);

            VkDescriptorSetLayout GetSetLayout() const;
            const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]; }

        private:
            const Rhi::RenderContext& m_context;
            Resource::RenderResources& m_resources;
            const Rhi::SwapChain& m_swapChain;
            Resource::DescriptorManager& m_descriptorMgr;

            std::array<Resource::RenderBufferHandle, Rhi::kMaxFramesInFlight> m_uboHandles{};
            std::array<VkDescriptorSet, Rhi::kMaxFramesInFlight> m_sets{};
        };
    }
}
