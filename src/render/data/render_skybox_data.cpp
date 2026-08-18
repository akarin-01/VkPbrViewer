#include "render_skybox_data.h"

#include "core/log.h"
#include "render/render_context.h"
#include "render/render_utils.h"
#include "render/render_resources.h"
#include "render/descriptor_allocator.h"
#include "render/compute_pipeline.h"
#include "scene/skybox.h"

#include <array>
#include <algorithm>
#include <cmath>

namespace Kita::Pbrv
{
    RenderSkyboxData::RenderSkyboxData(const RenderContext& context,
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

        // Sets (contents are written once the cubemap is implemented)
        for (auto& set : m_sets)
        {
            set = m_descriptorAllocator.Allocate(m_setLayout, "Skybox set");
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
            builder.SetShader("assets/shaders/equirect_to_cubemap_comp.spv")
                .SetDescriptorSetLayouts({ m_conversionSetLayout });
            m_conversionPipeline = builder.Build();
        }
    }

    RenderSkyboxData::~RenderSkyboxData()
    {
        // Sets will be destroyed automatically

        // Texture (only created once the cubemap is implemented; destroying zero handles is a safe no-op)
        m_resources.DestroySampler(m_cubemap.m_samplerHandle);
        m_resources.DestroyImageView(m_cubemap.m_imageViewHandle);
        m_resources.DestroyImage(m_cubemap.m_imageHandle);

        m_conversionPipeline.reset();
        vkDestroyDescriptorSetLayout(m_context.Device(), m_conversionSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(m_context.Device(), m_setLayout, nullptr);
    }

    void RenderSkyboxData::Update(uint32_t frameIndex, const Skybox& sceneSkybox)
    {
        if (sceneSkybox.IsDirty())
        {
            sceneSkybox.ClearDirty();

            DestroyTexture(m_cubemap);

            if (sceneSkybox.IsEmpty())
            {
                return;
            }

            m_cubemap = CreateCubemap(sceneSkybox);
            m_setRefreshCount = kMaxFramesInFlight;
            KITA_LOG_DEBUG("[Renderer] Update skybox descriptor set: textures changed");
        }

        bool setRefreshed = m_setRefreshCount > 0;
        if (setRefreshed)
        {
            WriteSet(m_sets[frameIndex]);
            --m_setRefreshCount;
        }
    }

    RenderTexture RenderSkyboxData::CreateCubemap(const Skybox& sceneSkybox) const
    {
        const VkFormat hdrFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
        const uint32_t faceSize = 512;

        // 1. Create equirect texture
        RenderTexture equirect = CreateEquirectTexture(sceneSkybox, hdrFormat);

        // 2. Create cubemap (storage and sample)
        RenderTexture cubemap = CreateCubemapTexture(faceSize, hdrFormat);

        // 3. GPU conversion: dispatch equirect_to_cubemap compute shader
        WriteConversionSet(equirect, cubemap);
        DispatchConversion(cubemap, faceSize);

        // 4. Destroy equirect texture
        DestroyTexture(equirect);

        Log::Info("[Renderer] Convert skybox cubemap: ", sceneSkybox.GetName(), ", ",
            sceneSkybox.GetWidth(), "x", sceneSkybox.GetHeight(), " -> ",
            faceSize, "x", faceSize, "x6");

        return cubemap;
    }

    void RenderSkyboxData::WriteSet(VkDescriptorSet set) const
    {
        VkDescriptorImageInfo imageInfo{};
        {
            RenderImageView* imageView = m_resources.GetImageView(m_cubemap.m_imageViewHandle);
            assert(imageView && "Image view handle is invalid");
            RenderSampler* sampler = m_resources.GetSampler(m_cubemap.m_samplerHandle);
            assert(sampler && "Sampler handle is invalid");

            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = imageView->m_imageView;
            imageInfo.sampler = sampler->m_sampler;
        }

        std::array<VkWriteDescriptorSet, 1> writes{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = set;
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].descriptorCount = 1;
        writes[0].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_context.Device(),
            static_cast<uint32_t>(writes.size()), writes.data(),
            0, nullptr);
    }

    RenderTexture RenderSkyboxData::CreateEquirectTexture(const Skybox& sceneSkybox, VkFormat format) const
    {
        RenderTexture equirect{};

        const auto& pixels = sceneSkybox.GetPixels();
        const uint32_t width = sceneSkybox.GetWidth();
        const uint32_t height = sceneSkybox.GetHeight();

        const uint32_t channels = static_cast<uint32_t>(pixels.size() / (width * height));
        assert(channels == 4 && "Skybox pixels must be RGBA (LoadSkybox forces 4 channels)");

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = { width, height, 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        equirect.m_imageHandle = m_resources.CreateImageWithData(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            pixels.data(), pixels.size() * sizeof(float));

        RenderImage* image = m_resources.GetImage(equirect.m_imageHandle);
        assert(image && "Equirect image view handle is invalid");

        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.image = image->m_image;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = format;
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;

        equirect.m_imageViewHandle = m_resources.CreateImageView(imageViewInfo);

        equirect.m_samplerHandle = m_resources.CreateSamplerLinearRepeatMip();

        return equirect;
    }

    RenderTexture RenderSkyboxData::CreateCubemapTexture(uint32_t faceSize, VkFormat format) const
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

        RenderImage* image = m_resources.GetImage(cubemap.m_imageHandle);
        assert(image && "Cubemap image view handle is invalid");

        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.image = image->m_image;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        imageViewInfo.format = format;
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 6;

        cubemap.m_imageViewHandle = m_resources.CreateImageView(imageViewInfo);

        cubemap.m_samplerHandle = m_resources.CreateSamplerLinearClampNoMip();

        return cubemap;
    }

    void RenderSkyboxData::DestroyTexture(RenderTexture& texture) const
    {
        m_resources.DestroySampler(texture.m_samplerHandle);
        m_resources.DestroyImageView(texture.m_imageViewHandle);
        m_resources.DestroyImage(texture.m_imageHandle);

        texture = {};
    }

    void RenderSkyboxData::WriteConversionSet(const RenderTexture& equirect, const RenderTexture& cubemap) const
    {
        VkDescriptorImageInfo cubemapInfo{};
        {
            RenderImageView* imageView = m_resources.GetImageView(cubemap.m_imageViewHandle);
            assert(imageView && "Cubemap image view handle is invalid");

            cubemapInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            cubemapInfo.imageView = imageView->m_imageView;
        }

        VkDescriptorImageInfo equirectInfo{};
        {
            RenderImageView* imageView = m_resources.GetImageView(equirect.m_imageViewHandle);
            assert(imageView && "Equirect image view handle is invalid");
            RenderSampler* sampler = m_resources.GetSampler(equirect.m_samplerHandle);
            assert(sampler && "Equirect sampler handle is invalid");

            equirectInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            equirectInfo.imageView = imageView->m_imageView;
            equirectInfo.sampler = sampler->m_sampler;
        }

        std::array<VkWriteDescriptorSet, 2> writes{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = m_conversionSet;
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[0].descriptorCount = 1;
        writes[0].pImageInfo = &cubemapInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = m_conversionSet;
        writes[1].dstBinding = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &equirectInfo;

        vkUpdateDescriptorSets(m_context.Device(),
            static_cast<uint32_t>(writes.size()), writes.data(),
            0, nullptr);
    }

    void RenderSkyboxData::DispatchConversion(const RenderTexture& cubemap, uint32_t faceSize) const
    {
        assert(m_conversionPipeline && "Conversion: pipeline is null");

        VkCommandBuffer commandBuffer = BeginSingleTimeCommands(m_context.Device(), m_context.CommandPool());

        VkImageSubresourceRange cubemapRange{};
        cubemapRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        cubemapRange.baseMipLevel = 0;
        cubemapRange.levelCount = 1;
        cubemapRange.baseArrayLayer = 0;
        cubemapRange.layerCount = 6;

        RenderImage* image = m_resources.GetImage(cubemap.m_imageHandle);
        assert(image && "Cubemap image handle is invalid");

        TransitionImageLayout(commandBuffer, image->m_image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, cubemapRange);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_conversionPipeline->Handle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_conversionPipeline->Layout(),
            0, 1, &m_conversionSet, 0, nullptr);
        vkCmdDispatch(commandBuffer, (faceSize + 7) / 8, (faceSize + 7) / 8, 6);

        TransitionImageLayout(commandBuffer, image->m_image,
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, cubemapRange);

        EndSingleTimeCommands(m_context.Device(), m_context.CommandPool(), m_context.GraphicsQueue(), commandBuffer);
    }
}
