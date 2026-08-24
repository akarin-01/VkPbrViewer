#include "render_ibl_data.h"

#include "core/log.h"
#include "render/render_context.h"
#include "render/render_resources.h"
#include "render/compute_conversion.h"
#include "render/render_texture_set.h"
#include "render/render_texture_utils.h"

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
        RenderTexture irradiance = CreateCubemapTexture(kIrradianceFaceSize, format);

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

    RenderTexture RenderIblData::CreateCubemapTexture(uint32_t faceSize, VkFormat format) const
    {
        RenderTexture cubemap{};

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = { faceSize, faceSize, 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 6;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        cubemap.m_imageHandle = m_resources.CreateImage(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        cubemap.m_imageViewHandle = m_resources.CreateImageView(cubemap.m_imageHandle, VK_IMAGE_VIEW_TYPE_CUBE);
        cubemap.m_samplerHandle = m_resources.CreateSamplerLinearClampNoMip();

        return cubemap;
    }
}
