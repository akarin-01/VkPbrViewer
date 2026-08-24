#include "render_texture_utils.h"

#include "render/render_resources.h"

namespace Kita::Pbrv
{
    RenderTexture CreateCubemapFallback(RenderResources& resources, VkFormat format)
    {
        RenderTexture fallback{};

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = { 1, 1, 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 6;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        // All-zero data is black in both R16 and R32 float formats
        std::array<uint16_t, 24> black{};   // 6 layers * 1 texel * RGBA16
        fallback.m_imageHandle = resources.CreateImageWithData(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            black.data(), black.size() * sizeof(uint16_t));

        resources.TransitionImageLayout(fallback.m_imageHandle,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);

        fallback.m_imageViewHandle = resources.CreateImageView(fallback.m_imageHandle, VK_IMAGE_VIEW_TYPE_CUBE);
        fallback.m_samplerHandle = resources.CreateSamplerLinearClampNoMip();

        return fallback;
    }

    void DestroyTexture(RenderResources& resources, RenderTexture& texture)
    {
        resources.DestroySampler(texture.m_samplerHandle);
        resources.DestroyImageView(texture.m_imageViewHandle);
        resources.DestroyImage(texture.m_imageHandle);

        texture = {};
    }
}
