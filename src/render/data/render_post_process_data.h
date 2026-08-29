#pragma once

#include "core/macro.h"
#include "rhi/constants.h"
#include "resource/render_texture.h"

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
        class Context;
    }
    namespace Resource
    {
        class Resources;
        class DescriptorManager;
    }

    namespace Render
    {
        struct PostProcessUbo
        {
            alignas(16) glm::vec4 m_exposure;      // x - exposure, yzw - padding
        };
        STD140_ASSERT(PostProcessUbo, 16);

        /// Set 1 — per-pass state of the post process: exposure UBO + the lit
        /// output texture. The texture swaps on resize (K-slot rotation).
        class RenderPostProcessData
        {
        public:
            RenderPostProcessData(const Rhi::Context& context,
                Resource::Resources& resources,
                Resource::DescriptorManager& descriptorMgr,
                const Resource::RenderTexture& target);
            ~RenderPostProcessData();

            /// Continuous: exposure is rewritten every frame.
            void UpdateUbo(uint32_t frameIndex, const Scene::PostProcess& postProcess);

            /// Discrete: the lit output swapped on resize (K-slot rotation).
            void UpdateTarget(const Resource::RenderTexture& target);
            void RefreshSet(uint32_t frameIndex);

            VkDescriptorSetLayout GetSetLayout() const;
            const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]; }

        private:
            void WriteSet(uint32_t frameIndex, bool writeUbo = false) const;

        private:
            const Rhi::Context& m_context;
            Resource::Resources& m_resources;
            Resource::DescriptorManager& m_descriptorMgr;

            std::array<Resource::RenderBufferHandle, Rhi::kMaxFramesInFlight> m_uboHandles{};
            Resource::RenderTexture m_offlineTex{};
            std::array<VkDescriptorSet, Rhi::kMaxFramesInFlight> m_sets{};
            uint32_t m_setDirtyCount{ 0 };
        };
    }
}
