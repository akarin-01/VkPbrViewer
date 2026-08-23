#include "render_ibl_data.h"

#include "core/log.h"
#include "render/render_context.h"
#include "render/render_utils.h"
#include "render/render_resources.h"
#include "render/descriptor_allocator.h"
#include "render/descriptor_writer.h"
#include "render/compute_pipeline.h"
#include "render/one_shot_command.h"

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

        // Conversion set layout
        {
            std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
            bindings[0].binding = 0;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            bindings[0].descriptorCount = 1;
            bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

            bindings[1].binding = 1;
            bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[1].descriptorCount = 1;
            bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

            VkDescriptorSetLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            createInfo.pBindings = bindings.data();

            m_conversionSetLayout = CreateDescriptorSetLayout(m_context.Device(), createInfo);
        }

        // Conversion set
        {
            m_conversionSet = m_descriptorAllocator.Allocate(m_conversionSetLayout, "Conversion set");
        }

        // Conversion pipeline
        {
            ComputePipelineBuilder builder(m_context.Device());
            builder.SetShader("assets/shaders/irradiance_convolution_comp.spv")
                .SetDescriptorSetLayouts({ m_conversionSetLayout });
            m_conversionPipeline = builder.Build();
        }
    }

    RenderIblData::~RenderIblData()
    {
        m_conversionPipeline.reset();

        // Sets will be destroyed automatically

        // Texture
        DestroyTexture(m_irradianceMap);

        DestroyFallback();

        vkDestroyDescriptorSetLayout(m_context.Device(), m_conversionSetLayout, nullptr);
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

    RenderTexture RenderIblData::CreateIrradianceMap(const RenderTexture& envmap) const
    {
        const uint32_t faceSize = 32;

        // 1. Create irradiance map
        RenderTexture irradiance = CreateIrradianceTexture(faceSize);

        // 2. GPU conversion: dispatch irradiance_convolution compute shader
        WriteConversionSet(irradiance, envmap);
        DispatchConversion(irradiance, envmap);

        return irradiance;
    }

    RenderTexture RenderIblData::CreateIrradianceTexture(uint32_t faceSize) const
    {
        RenderTexture irradianceTex{};

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = { faceSize, faceSize, 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 6;
        imageInfo.format = m_context.HdrFormat();
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        irradianceTex.m_imageHandle = m_resources.CreateImage(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        irradianceTex.m_imageViewHandle = m_resources.CreateImageView(irradianceTex.m_imageHandle, VK_IMAGE_VIEW_TYPE_CUBE);

        irradianceTex.m_samplerHandle = m_resources.CreateSamplerLinearClampNoMip();

        return irradianceTex;
    }

    void RenderIblData::WriteConversionSet(const RenderTexture& irradiance, const RenderTexture& envMap) const
    {
        DescriptorWriter writer(m_resources, m_context.Device());
        writer.WriteImage(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            VK_IMAGE_LAYOUT_GENERAL, irradiance.m_imageViewHandle, 0)
            .WriteImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, envMap.m_imageViewHandle, envMap.m_samplerHandle)
            .UpdateSet(m_conversionSet);
    }

    void RenderIblData::DispatchConversion(const RenderTexture& irradiance, const RenderTexture& envMap) const
    {
        assert(m_conversionPipeline && "Conversion: pipeline is null");

        {
            OneShotCommand command(m_context);

            RenderImage* irradianceImage = m_resources.GetImage(irradiance.m_imageHandle);
            assert(irradianceImage && "Irradiance image handle is invalid");
            RenderImage* envMapImage = m_resources.GetImage(envMap.m_imageHandle);
            assert(envMapImage && "Env map image handle is invalid");

            uint32_t irradianceWidth = irradianceImage->m_extent.width;
            uint32_t irradianceHeight = irradianceImage->m_extent.height;

            VkImageSubresourceRange irradianceRange{};
            irradianceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            irradianceRange.baseMipLevel = 0;
            irradianceRange.levelCount = 1;
            irradianceRange.baseArrayLayer = 0;
            irradianceRange.layerCount = 6;

            // Irradiance: undefined -> general
            TransitionImageLayout(command.Handle(), irradianceImage->m_image,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                irradianceRange);

            vkCmdBindPipeline(command.Handle(), VK_PIPELINE_BIND_POINT_COMPUTE, m_conversionPipeline->Handle());
            vkCmdBindDescriptorSets(command.Handle(), VK_PIPELINE_BIND_POINT_COMPUTE, m_conversionPipeline->Layout(),
                0, 1, &m_conversionSet, 0, nullptr);
            vkCmdDispatch(command.Handle(), (irradianceWidth + 7) / 8, (irradianceHeight + 7) / 8, 6);

            // Irradiance: general -> shader read only
            TransitionImageLayout(command.Handle(), irradianceImage->m_image,
                VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
                irradianceRange);
        }
    }
}
