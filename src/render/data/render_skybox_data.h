#pragma once

#include "render/render_resource_types.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <array>

namespace Kita::Pbrv
{
    class RenderContext;
    class RenderResources;
    class DescriptorAllocator;
    class Skybox;
    class ComputePipeline;

    class RenderSkyboxData
    {
    public:
        RenderSkyboxData(const RenderContext& context,
            RenderResources& resources,
            const DescriptorAllocator& descriptorAllocator);
        ~RenderSkyboxData();

        void Update(uint32_t frameIndex, const Skybox& sceneSkybox);

        bool IsReady() const { return m_cubemap.m_imageHandle != 0; }

        VkDescriptorSetLayout GetSetLayout() const { return m_setLayout; }
        const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]; }

    private:
        RenderTexture CreateCubemap(const Skybox& sceneSkybox) const;
        void WriteSet(VkDescriptorSet set) const;

        RenderTexture CreateEquirectTexture(const Skybox& sceneSkybox, VkFormat format) const;
        RenderTexture CreateCubemapTexture(uint32_t faceSize, VkFormat format) const;
        void DestroyTexture(RenderTexture& texture) const;
        void WriteConversionSet(const RenderTexture& equirect, const RenderTexture& cubemap) const;
        void DispatchConversion(const RenderTexture& cubemap, uint32_t faceSize) const;

    private:
        const RenderContext& m_context;
        RenderResources& m_resources;
        const DescriptorAllocator& m_descriptorAllocator;

        RenderTexture m_cubemap{};
        VkDescriptorSetLayout m_setLayout{ VK_NULL_HANDLE };
        std::array<VkDescriptorSet, kMaxFramesInFlight> m_sets{};
        uint32_t m_setRefreshCount{ 0 };

        // GPU conversion: equirect -> cubemap
        VkDescriptorSetLayout m_conversionSetLayout{ VK_NULL_HANDLE };
        VkDescriptorSet m_conversionSet{ VK_NULL_HANDLE };
        std::unique_ptr<ComputePipeline> m_conversionPipeline;
    };
}
