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
        class PostProcess;
    }
    namespace Rhi
    {
        class RenderContext;
    }
    namespace Resource
    {
        class RenderResources;
        class DescriptorManager;
    }

    namespace Render
    {
        struct PostProcessUbo
        {
            alignas(16) glm::vec4 m_exposure;          // x - exposure, yzw - padding
        };
        STD140_ASSERT(PostProcessUbo, 16);

        class RenderPostProcessData
        {
        public:
            RenderPostProcessData(const Rhi::RenderContext& context,
                Resource::RenderResources& resources,
                Resource::DescriptorManager& descriptorMgr);
            ~RenderPostProcessData();

            void Update(uint32_t frameIndex, const Scene::PostProcess& postProcess);

            VkDescriptorSetLayout GetSetLayout() const;
            const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]; }

        private:
            const Rhi::RenderContext& m_context;
            Resource::RenderResources& m_resources;
            Resource::DescriptorManager& m_descriptorMgr;

            std::array<Resource::RenderBufferHandle, Rhi::kMaxFramesInFlight> m_uboHandles{};
            std::array<VkDescriptorSet, Rhi::kMaxFramesInFlight> m_sets{};
        };
    }
}
