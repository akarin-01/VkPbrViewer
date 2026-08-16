#include "render_pipeline.h"

#include "render/render_resources.h"
#include "render/swap_chain.h"
#include "render/passes/lit_pass.h"
#include "render/passes/skybox_pass.h"
#include "render/passes/post_process_pass.h"

#include <cassert>

namespace Kita::Pbrv
{
    RenderPipeline::RenderPipeline(const RenderContext& context,
        RenderResources& resources,
        const SwapChain& swapChain,
        const DescriptorAllocator& descriptorAllocator,
        const RenderList& list)
        : m_resources(resources),
        m_swapChain(swapChain)
    {
        CreateRenderTarget();
        CreateRenderPasses(context, resources, swapChain, descriptorAllocator, list);
    }

    RenderPipeline::~RenderPipeline()
    {
        DestroyRenderPasses();
        DestroyRenderTarget();
    }

    void RenderPipeline::RecreateResources()
    {
        RecreateRenderTarget();

        for (auto& pass : m_passes)
        {
            pass->RecreateResources();
        }
    }

    void RenderPipeline::Draw(const RenderList& list, const FrameInfo& frameInfo) const
    {
        for (auto& pass : m_passes)
        {
            pass->Draw(list, frameInfo);
        }
    }

    void RenderPipeline::CreateRenderTarget()
    {
        VkExtent2D extent = m_swapChain.Extent();

        m_target.m_colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        // Color texture
        {
            auto& tex = m_target.m_colorTex;
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.format = m_target.m_colorFormat;
            imageInfo.extent = { extent.width, extent.height, 1 };
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            tex.m_imageHandle = m_resources.CreateImage(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            RenderImage* image = m_resources.GetImage(tex.m_imageHandle);
            assert(image && "Render target color image handle is invalid");

            VkImageViewCreateInfo imageViewInfo{};
            imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewInfo.image = image->m_image;
            imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewInfo.format = m_target.m_colorFormat;
            imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewInfo.subresourceRange.baseMipLevel = 0;
            imageViewInfo.subresourceRange.levelCount = 1;
            imageViewInfo.subresourceRange.baseArrayLayer = 0;
            imageViewInfo.subresourceRange.layerCount = 1;
            tex.m_imageViewHandle = m_resources.CreateImageView(imageViewInfo);

            tex.m_samplerHandle = m_resources.CreateSamplerLinearClampNoMip();
        }

        m_target.m_depthFormat = VK_FORMAT_D32_SFLOAT;      // m_context.GetDepthFormat()
        // Depth texture
        {
            auto& tex = m_target.m_depthTex;
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent = { extent.width, extent.height, 1 };
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = m_target.m_depthFormat;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            tex.m_imageHandle = m_resources.CreateImage(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            RenderImage* image = m_resources.GetImage(tex.m_imageHandle);
            assert(image && "Render target depth image handle is invalid");

            VkImageViewCreateInfo imageViewInfo{};
            imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewInfo.image = image->m_image;
            imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewInfo.format = m_target.m_depthFormat;
            imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            imageViewInfo.subresourceRange.baseMipLevel = 0;
            imageViewInfo.subresourceRange.levelCount = 1;
            imageViewInfo.subresourceRange.baseArrayLayer = 0;
            imageViewInfo.subresourceRange.layerCount = 1;
            tex.m_imageViewHandle = m_resources.CreateImageView(imageViewInfo);

            tex.m_samplerHandle = m_resources.CreateSamplerNearestClampNoMip();
        }
    }

    void RenderPipeline::DestroyRenderTarget()
    {
        // Color texture
        {
            auto& tex = m_target.m_colorTex;
            m_resources.DestroySampler(tex.m_samplerHandle);
            m_resources.DestroyImageView(tex.m_imageViewHandle);
            m_resources.DestroyImage(tex.m_imageHandle);
        }
        // Depth texture
        {
            auto& tex = m_target.m_depthTex;
            m_resources.DestroySampler(tex.m_samplerHandle);
            m_resources.DestroyImageView(tex.m_imageViewHandle);
            m_resources.DestroyImage(tex.m_imageHandle);
        }

        m_target = {};
    }

    void RenderPipeline::RecreateRenderTarget()
    {
        DestroyRenderTarget();
        CreateRenderTarget();
    }

    void RenderPipeline::CreateRenderPasses(const RenderContext& context,
        RenderResources& resources,
        const SwapChain& swapChain,
        const DescriptorAllocator& descriptorAllocator,
        const RenderList& list)
    {
        m_passes.push_back(std::make_unique<LitPass>(
            context, resources, swapChain, descriptorAllocator, list, m_target));
        m_passes.push_back(std::make_unique<SkyboxPass>(
            context, resources, swapChain, descriptorAllocator, list, m_target));
        m_passes.push_back(std::make_unique<PostProcessPass>(
            context, resources, swapChain, descriptorAllocator, list, m_target));
    }

    void RenderPipeline::DestroyRenderPasses()
    {
        m_passes.clear();
    }
}