#include "render_skybox_data.h"

#include "core/log.h"
#include "scene/skybox.h"
#include "render/render_context.h"
#include "render/render_utils.h"
#include "render/render_resources.h"
#include "render/descriptor_allocator.h"
#include "render/descriptor_writer.h"
#include "render/compute_conversion.h"

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

        // Sets
        for (auto& set : m_sets)
        {
            set = m_descriptorAllocator.Allocate(m_setLayout, "Skybox set");
        }

        m_conversion = std::make_unique<ComputeConversion>(m_context, m_resources, m_descriptorAllocator,
            1, "assets/shaders/equirect_to_cubemap_comp.spv", 0);
    }

    RenderSkyboxData::~RenderSkyboxData()
    {
        m_conversion.reset();

        // Sets will be destroyed automatically

        // Texture
        DestroyTexture(m_cubemap);

        vkDestroyDescriptorSetLayout(m_context.Device(), m_setLayout, nullptr);
    }

    void RenderSkyboxData::Update(uint32_t frameIndex, const Skybox& sceneSkybox)
    {
        if (sceneSkybox.GetRevision() != m_lastSyncedRevision)
        {
            m_lastSyncedRevision = sceneSkybox.GetRevision();

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
        const uint32_t faceSize = 2048;
        const VkFormat equirectFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

        // 1. Create equirect texture (SHADER_READ_ONLY_OPTIMAL)
        RenderTexture equirect = CreateEquirectTexture(sceneSkybox, equirectFormat);
        m_resources.TransitionImageLayout(equirect.m_imageHandle,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);

        // 2.1 Create cubemap (sample)
        RenderTexture cubemap = CreateCubemapTexture(faceSize, m_context.HdrFormat());

        // 2.2 Create storage image view for conversion
        VkImageSubresourceRange storageRange{};
        storageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        storageRange.baseMipLevel = 0;
        storageRange.levelCount = 1;
        storageRange.baseArrayLayer = 0;
        storageRange.layerCount = 6;
        RenderImageViewHandle storageImageViewHandle = m_resources.CreateImageView(cubemap.m_imageHandle, VK_IMAGE_VIEW_TYPE_CUBE, storageRange);

        // 3. GPU conversion: dispatch equirect_to_cubemap compute shader
        VkImageSubresourceRange range{};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 6;

        ComputeConversion::Output output{};
        output.m_image = cubemap.m_imageHandle;
        output.m_imageView = storageImageViewHandle;
        output.m_range = range;
        output.m_finalLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        output.m_finalStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        output.m_finalAccess = VK_ACCESS_2_TRANSFER_WRITE_BIT;

        ComputeConversion::Input input{};
        input.m_imageView = equirect.m_imageViewHandle;
        input.m_sampler = equirect.m_samplerHandle;

        m_conversion->Dispatch(output, { (faceSize + 7) / 8, (faceSize + 7) / 8, 6 },
            { input });

        RenderImage* cubemapImage = m_resources.GetImage(cubemap.m_imageHandle);
        assert(cubemapImage && "Skybox: cubemap image is invalid handle");
        range.baseMipLevel = 1;
        range.levelCount = cubemapImage->m_mipLevels - 1;
        m_resources.TransitionImageLayout(cubemap.m_imageHandle,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            range);

        // 4. Destroy conversion resources
        m_resources.DestroyImageView(storageImageViewHandle);
        DestroyTexture(equirect);

        // 5. Generate cubemap mipmaps
        m_resources.GenerateImageMipmaps(cubemap.m_imageHandle);

        Log::Info("[Renderer] Convert skybox cubemap: ", sceneSkybox.GetName(), ", ",
            sceneSkybox.GetWidth(), "x", sceneSkybox.GetHeight(), " -> ",
            faceSize, "x", faceSize, "x6");

        return cubemap;
    }

    void RenderSkyboxData::WriteSet(VkDescriptorSet set) const
    {
        DescriptorWriter writer(m_resources, m_context.Device());
        writer.WriteImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_cubemap.m_imageViewHandle, m_cubemap.m_samplerHandle)
            .UpdateSet(set);
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

        equirect.m_imageViewHandle = m_resources.CreateImageView(equirect.m_imageHandle);

        equirect.m_samplerHandle = m_resources.CreateSamplerEquirect();

        return equirect;
    }

    RenderTexture RenderSkyboxData::CreateCubemapTexture(uint32_t faceSize, VkFormat format) const
    {
        auto mipLevels = CalculateMipLevels(faceSize, faceSize);

        RenderTexture cubemap{};

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = { faceSize, faceSize, 1 };
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 6;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        cubemap.m_imageHandle = m_resources.CreateImage(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        cubemap.m_imageViewHandle = m_resources.CreateImageView(cubemap.m_imageHandle, VK_IMAGE_VIEW_TYPE_CUBE);

        cubemap.m_samplerHandle = m_resources.CreateSamplerLinearClampMip();

        return cubemap;
    }

    void RenderSkyboxData::DestroyTexture(RenderTexture& texture) const
    {
        m_resources.DestroySampler(texture.m_samplerHandle);
        m_resources.DestroyImageView(texture.m_imageViewHandle);
        m_resources.DestroyImage(texture.m_imageHandle);

        texture = {};
    }
}
