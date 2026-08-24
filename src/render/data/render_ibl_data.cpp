#include "render_ibl_data.h"

#include "core/log.h"
#include "render/render_context.h"
#include "render/render_resources.h"
#include "render/compute_conversion.h"
#include "render/render_texture_set.h"

#include <cassert>

namespace Kita::Pbrv
{
    namespace
    {
        constexpr uint32_t kIrradianceFaceSize = 32;
    }

    RenderIblData::RenderIblData(const RenderContext& context,
        RenderResources& resources,
        const DescriptorAllocator& descriptorAllocator)
        : m_context(context),
        m_resources(resources),
        m_descriptorAllocator(descriptorAllocator)
    {
        m_textureSet = std::make_unique<TextureSet>(m_context, m_resources, m_descriptorAllocator,
            TextureArray{ CreateCubemapFallback(m_resources, m_context.HdrFormat()) });
        m_conversion = std::make_unique<ComputeConversion>(m_context, m_resources, m_descriptorAllocator,
            1, "assets/shaders/irradiance_convolution_comp.spv", 0);
    }

    RenderIblData::~RenderIblData() = default;

    void RenderIblData::Update(uint32_t frameIndex, const RenderTexture& sourceCubemap)
    {
        if (m_lastCubemap != sourceCubemap)
        {
            m_lastCubemap = sourceCubemap;

            // Source changed: convolve a new irradiance map, or restore the placeholder
            TextureArray updatedTexs{};

            if (sourceCubemap.IsEmpty())
            {
                updatedTexs = m_textureSet->GetFallbacks();
            }
            else
            {
                updatedTexs[0] = CreateIrradianceMap(sourceCubemap);

                Log::Info("[Renderer] Create irradiance map: ",
                    kIrradianceFaceSize, "x", kIrradianceFaceSize, "x6");
            }

            m_textureSet->Update(updatedTexs);
            KITA_LOG_DEBUG("[Renderer] Update IBL descriptor set: textures changed");
        }

        m_textureSet->RefreshSet(frameIndex);
    }

    VkDescriptorSetLayout RenderIblData::GetSetLayout() const
    {
        return m_textureSet->GetLayout();
    }

    const VkDescriptorSet& RenderIblData::GetSet(uint32_t frameIndex) const
    {
        return m_textureSet->GetSet(frameIndex);
    }

    RenderTexture RenderIblData::CreateIrradianceMap(const RenderTexture& sourceCubemap) const
    {
        const VkFormat format = m_context.HdrFormat();

        // 1. Create irradiance map
        RenderTexture irradiance = CreateCubemapTexture(m_resources,
            kIrradianceFaceSize, format, 1, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            m_resources.CreateSamplerLinearClampNoMip());

        // 2. GPU conversion: dispatch irradiance_convolution compute shader
        VkImageSubresourceRange range{};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 6;

        ComputeConversion::Output output{};
        output.m_image = irradiance.m_imageHandle;
        output.m_imageView = irradiance.m_imageViewHandle;
        output.m_range = range;

        ComputeConversion::Input input{};
        input.m_imageView = sourceCubemap.m_imageViewHandle;
        input.m_sampler = sourceCubemap.m_samplerHandle;

        m_conversion->Dispatch(output, { (kIrradianceFaceSize + 7) / 8, (kIrradianceFaceSize + 7) / 8, 6 },
            { input });

        return irradiance;
    }
}
