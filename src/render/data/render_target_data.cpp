#include "render_target_data.h"

#include "render/render_context.h"
#include "render/render_utils.h"
#include "render/render_resources.h"
#include "render/descriptor_allocator.h"
#include "render/descriptor_writer.h"

#include <array>
#include <cassert>

namespace Kita::Pbrv
{
    RenderTargetData::RenderTargetData(const RenderContext& context,
        RenderResources& resources,
        const DescriptorAllocator& descriptorAllocator,
        VkExtent2D extent)
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

        // Set
        {
            m_set = m_descriptorAllocator.Allocate(m_setLayout, "Target set");
        }

        Create(extent);
    }

    RenderTargetData::~RenderTargetData()
    {
        Destroy();

        vkDestroyDescriptorSetLayout(m_context.Device(), m_setLayout, nullptr);
    }

    void RenderTargetData::Recreate(VkExtent2D extent)
    {
        Destroy();
        Create(extent);
    }

    VkImageView RenderTargetData::GetColorImageView() const
    {
        RenderImageView* imageView = m_resources.GetImageView(m_colorTex.m_imageViewHandle);
        assert(imageView && "Color image view handle is invalid");

        return imageView->m_imageView;
    }

    VkFormat RenderTargetData::GetColorFormat() const
    {
        RenderImage* image = m_resources.GetImage(m_colorTex.m_imageHandle);
        assert(image && "Color image handle is invalid");

        return image->m_format;
    }

    VkImageView RenderTargetData::GetDepthImageView() const
    {
        RenderImageView* imageView = m_resources.GetImageView(m_depthTex.m_imageViewHandle);
        assert(imageView && "Depth image view handle is invalid");

        return imageView->m_imageView;
    }

    VkFormat RenderTargetData::GetDepthFormat() const
    {
        RenderImage* image = m_resources.GetImage(m_depthTex.m_imageHandle);
        assert(image && "Depth image handle is invalid");

        return image->m_format;
    }

    void RenderTargetData::TransitionColorImageLayout(VkCommandBuffer commandBuffer,
        VkImageLayout newLayout,
        VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
        VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask)
    {
        if (m_colorLayout == newLayout)
        {
            return;
        }

        VkImageSubresourceRange range{};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        RenderImage* image = m_resources.GetImage(m_colorTex.m_imageHandle);
        assert(image && "Color image handle is invalid");
        TransitionImageLayout(commandBuffer,
            image->m_image,
            m_colorLayout, newLayout,
            srcStageMask, srcAccessMask,
            dstStageMask, dstAccessMask,
            range);

        m_colorLayout = newLayout;
    }

    void RenderTargetData::TransitionDepthImageLayout(VkCommandBuffer commandBuffer,
        VkImageLayout newLayout,
        VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
        VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask)
    {
        if (m_depthLayout == newLayout)
        {
            return;
        }

        VkImageSubresourceRange range{};
        range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        RenderImage* image = m_resources.GetImage(m_depthTex.m_imageHandle);
        assert(image && "Depth image handle is invalid");
        TransitionImageLayout(commandBuffer,
            image->m_image,
            m_depthLayout, newLayout,
            srcStageMask, srcAccessMask,
            dstStageMask, dstAccessMask,
            range);

        m_depthLayout = newLayout;
    }

    void RenderTargetData::Create(VkExtent2D extent)
    {
        // Color texture
        {
            m_colorLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.format = m_context.HdrFormat();
            imageInfo.extent = { extent.width, extent.height, 1 };
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.samples = m_context.SampleCount();
            imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            m_colorTex.m_imageHandle = m_resources.CreateImage(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            m_colorTex.m_imageViewHandle = m_resources.CreateImageView(m_colorTex.m_imageHandle);

            m_colorTex.m_samplerHandle = m_resources.CreateSamplerLinearClampNoMip();
        }

        // Depth texture
        {
            m_depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent = { extent.width, extent.height, 1 };
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = m_context.DepthFormat();
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            imageInfo.samples = m_context.SampleCount();
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            m_depthTex.m_imageHandle = m_resources.CreateImage(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            m_depthTex.m_imageViewHandle = m_resources.CreateImageView(m_depthTex.m_imageHandle, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT);

            m_depthTex.m_samplerHandle = m_resources.CreateSamplerNearestClampNoMip();
        }

        // Set
        {
            DescriptorWriter writer(m_resources, m_context.Device());
            writer.WriteImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_colorTex.m_imageViewHandle, m_colorTex.m_samplerHandle)
                .UpdateSet(m_set);
        }
    }

    void RenderTargetData::Destroy()
    {
        // Color texture
        {
            m_resources.DestroySampler(m_colorTex.m_samplerHandle);
            m_resources.DestroyImageView(m_colorTex.m_imageViewHandle);
            m_resources.DestroyImage(m_colorTex.m_imageHandle);
        }
        // Depth texture
        {
            m_resources.DestroySampler(m_depthTex.m_samplerHandle);
            m_resources.DestroyImageView(m_depthTex.m_imageViewHandle);
            m_resources.DestroyImage(m_depthTex.m_imageHandle);
        }
    }
}
