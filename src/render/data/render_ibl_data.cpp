#include "render_ibl_data.h"

#include "core/log.h"
#include "render/render_context.h"
#include "render/render_utils.h"
#include "render/render_resources.h"
#include "render/descriptor_allocator.h"
#include "render/descriptor_writer.h"
#include "render/compute_conversion.h"

#include <cassert>

namespace Kita::Pbrv
{
    RenderIblData::RenderIblData(const RenderContext& context,
        RenderResources& resources,
        const DescriptorAllocator& descriptorAllocator)
        : m_context(context),
        m_resources(resources),
        m_descriptorAllocator(descriptorAllocator)
    {
        // Set layout
        {
            std::array<VkDescriptorSetLayoutBinding, 1> bindings{};
            bindings[0].binding = 0;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[0].descriptorCount = 1;
            bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            createInfo.pBindings = bindings.data();

            m_setLayout = CreateDescriptorSetLayout(m_context.Device(), createInfo);
        }

        CreateFallback();
        m_irradianceMap = m_fallback;

        // Sets
        for (auto& set : m_sets)
        {
            set = m_descriptorAllocator.Allocate(m_setLayout, "IBL set");
            WriteSet(set);
        }

        m_conversion = std::make_unique<ComputeConversion>(m_context, m_resources, m_descriptorAllocator,
            1, "assets/shaders/irradiance_convolution_comp.spv", 0);
    }

    RenderIblData::~RenderIblData()
    {
        m_conversion.reset();

        // Sets will be destroyed automatically

        // Texture
        DestroyTexture(m_irradianceMap);
        DestroyFallback();

        vkDestroyDescriptorSetLayout(m_context.Device(), m_setLayout, nullptr);
    }

    void RenderIblData::Update(uint32_t frameIndex, const RenderTexture& skyboxCubemap)
    {
        if (m_lastSkyboxCubemap != skyboxCubemap)
        {
            m_lastSkyboxCubemap = skyboxCubemap;

            bool isOldFallback = m_irradianceMap == m_fallback;
            if (!isOldFallback)
            {
                DestroyTexture(m_irradianceMap);
            }

            if (skyboxCubemap.m_imageHandle != 0)
            {
                m_irradianceMap = CreateIrradianceMap(skyboxCubemap);

                Log::Info("[Renderer] Update irradiance map: skybox cubemap changed");
            }
            else
            {
                m_irradianceMap = m_fallback;
            }

            m_setRefreshCount = kMaxFramesInFlight;
        }

        if (m_setRefreshCount > 0)
        {
            WriteSet(m_sets[frameIndex]);
            --m_setRefreshCount;
        }
    }

    void RenderIblData::CreateFallback()
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = { 1, 1, 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 6;
        imageInfo.format = m_context.HdrFormat();
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        // All-zero data is black in both R16 and R32 float formats
        std::array<uint16_t, 24> black{};   // 6 layers * 1 texel * RGBA16
        m_fallback.m_imageHandle = m_resources.CreateImageWithData(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            black.data(), black.size() * sizeof(uint16_t));

        // Fallback: TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
        m_resources.TransitionImageLayout(m_fallback.m_imageHandle,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);

        m_fallback.m_imageViewHandle = m_resources.CreateImageView(m_fallback.m_imageHandle, VK_IMAGE_VIEW_TYPE_CUBE);
        m_fallback.m_samplerHandle = m_resources.CreateSamplerLinearClampNoMip();
    }

    void RenderIblData::DestroyFallback()
    {
        DestroyTexture(m_fallback);
    }

    void RenderIblData::DestroyTexture(RenderTexture& texture) const
    {
        m_resources.DestroySampler(texture.m_samplerHandle);
        m_resources.DestroyImageView(texture.m_imageViewHandle);
        m_resources.DestroyImage(texture.m_imageHandle);

        texture = {};
    }

    void RenderIblData::WriteSet(VkDescriptorSet set) const
    {
        DescriptorWriter writer(m_resources, m_context.Device());
        writer.WriteImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_irradianceMap.m_imageViewHandle, m_irradianceMap.m_samplerHandle)
            .UpdateSet(set);
    }

    RenderTexture RenderIblData::CreateIrradianceMap(const RenderTexture& sourceCubemap) const
    {
        const uint32_t faceSize = 32;
        const VkFormat format = m_context.HdrFormat();

        // 1. Create irradiance map
        RenderTexture irradiance = CreateCubemapTexture(faceSize, format);

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

        m_conversion->Dispatch(output, { (faceSize + 7) / 8, (faceSize + 7) / 8, 6 },
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
