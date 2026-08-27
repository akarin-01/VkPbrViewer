#include "render_target_data.h"
#include "rhi/context.h"
#include "rhi/descriptor_writer.h"
#include "rhi/utils.h"
#include "resource/descriptor_manager.h"
#include "resource/resources.h"
#include "resource/render_texture.h"

#include <cassert>

namespace Kita::Pbrv
{
    namespace Render
    {
        namespace
        {
            constexpr Resource::DescriptorLayoutType kLayoutType = Resource::DescriptorLayoutType::TargetTex;
        }

        RenderTargetData::RenderTargetData(const Rhi::RenderContext& context,
            Resource::RenderResources& resources,
            Resource::DescriptorManager& descriptorMgr,
            VkExtent2D extent)
            : m_context(context),
            m_resources(resources),
            m_descriptorMgr(descriptorMgr)
        {
            // Set
            m_set = m_descriptorMgr.Allocate(kLayoutType);

            Create(extent);
        }

        RenderTargetData::~RenderTargetData()
        {
            Destroy();
        }

        VkDescriptorSetLayout RenderTargetData::GetSetLayout() const
        {
            return m_descriptorMgr.GetLayout(Resource::DescriptorLayoutType::TargetTex);
        }

        void RenderTargetData::Recreate(VkExtent2D extent)
        {
            Destroy();
            Create(extent);
        }

        VkImageView RenderTargetData::GetColorImageView() const
        {
            Resource::RenderImageView* imageView = m_resources.GetImageView(m_colorTex.m_imageViewHandle);
            assert(imageView && "Color image view handle is invalid");

            return imageView->m_imageView;
        }

        VkFormat RenderTargetData::GetColorFormat() const
        {
            Resource::RenderImage* image = m_resources.GetImage(m_colorTex.m_imageHandle);
            assert(image && "Color image handle is invalid");

            return image->m_format;
        }

        VkImageView RenderTargetData::GetResolveImageView() const
        {
            Resource::RenderImageView* imageView = m_resources.GetImageView(m_resolveTex.m_imageViewHandle);
            assert(imageView && "Resolve image view handle is invalid");

            return imageView->m_imageView;
        }

        VkFormat RenderTargetData::GetResolveFormat() const
        {
            Resource::RenderImage* image = m_resources.GetImage(m_resolveTex.m_imageHandle);
            assert(image && "Resolve image handle is invalid");

            return image->m_format;
        }

        VkImageView RenderTargetData::GetDepthImageView() const
        {
            Resource::RenderImageView* imageView = m_resources.GetImageView(m_depthTex.m_imageViewHandle);
            assert(imageView && "Depth image view handle is invalid");

            return imageView->m_imageView;
        }

        VkFormat RenderTargetData::GetDepthFormat() const
        {
            Resource::RenderImage* image = m_resources.GetImage(m_depthTex.m_imageHandle);
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

            Resource::RenderImage* image = m_resources.GetImage(m_colorTex.m_imageHandle);
            assert(image && "Color image handle is invalid");
            Rhi::TransitionImageLayout(commandBuffer,
                image->m_image,
                m_colorLayout, newLayout,
                srcStageMask, srcAccessMask,
                dstStageMask, dstAccessMask,
                range);

            m_colorLayout = newLayout;
        }

        void RenderTargetData::TransitionResolveImageLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask)
        {
            if (m_resolveLayout == newLayout)
            {
                return;
            }

            VkImageSubresourceRange range{};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = 1;

            Resource::RenderImage* image = m_resources.GetImage(m_resolveTex.m_imageHandle);
            assert(image && "Resolve image handle is invalid");
            Rhi::TransitionImageLayout(commandBuffer,
                image->m_image,
                m_resolveLayout, newLayout,
                srcStageMask, srcAccessMask,
                dstStageMask, dstAccessMask,
                range);

            m_resolveLayout = newLayout;
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

            Resource::RenderImage* image = m_resources.GetImage(m_depthTex.m_imageHandle);
            assert(image && "Depth image handle is invalid");
            Rhi::TransitionImageLayout(commandBuffer,
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
                imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

                m_colorTex.m_imageHandle = m_resources.CreateImage(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                m_colorTex.m_imageViewHandle = m_resources.CreateImageView(m_colorTex.m_imageHandle);
                m_colorTex.m_samplerHandle = m_resources.CreateSamplerLinearClampNoMip();
            }

            // Resolve texture
            {
                VkImageCreateInfo imageInfo{};
                imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
                imageInfo.format = m_context.HdrFormat();
                imageInfo.extent = { extent.width, extent.height, 1 };
                imageInfo.mipLevels = 1;
                imageInfo.arrayLayers = 1;
                imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

                m_resolveTex.m_imageHandle = m_resources.CreateImage(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                m_resolveTex.m_imageViewHandle = m_resources.CreateImageView(m_resolveTex.m_imageHandle);
                m_resolveTex.m_samplerHandle = m_resources.CreateSamplerLinearClampNoMip();
            }

            // Depth texture
            {
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
                Rhi::DescriptorWriter writer(m_resources, m_context.Device());
                writer.WriteImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_resolveTex.m_imageViewHandle, m_resolveTex.m_samplerHandle)
                    .UpdateSet(m_set);
            }
        }

        void RenderTargetData::Destroy()
        {
            // Color texture
            Resource::DestroyTexture(m_resources, m_colorTex);
            m_colorLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            // Resolve texture
            Resource::DestroyTexture(m_resources, m_resolveTex);
            m_resolveLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            // Depth texture
            Resource::DestroyTexture(m_resources, m_depthTex);
            m_depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }
    }
}
