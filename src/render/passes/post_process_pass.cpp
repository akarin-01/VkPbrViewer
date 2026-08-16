#include "post_process_pass.h"

#include "render/render_context.h"
#include "render/render_utils.h"
#include "render/render_resources.h"
#include "render/swap_chain.h"
#include "render/descriptor_allocator.h"
#include "render/graphics_pipeline.h"
#include "render/rendering_scope.h"

#include <stdexcept>

namespace Kita::Pbrv
{
    PostProcessPass::PostProcessPass(const RenderContext& context, RenderResources& resources, const SwapChain& swapChain, const DescriptorAllocator& descriptorAllocator, const RenderTarget& target)
        :RenderPassBase(context, resources, swapChain, descriptorAllocator, target)
    {
        CreateDescriptorSetLayouts();
        AllocateDescriptorSets();
        CreatePipeline();
    }

    PostProcessPass::~PostProcessPass()
    {
        m_pipeline.reset();
        vkDestroyDescriptorSetLayout(m_context.Device(), m_inputLayout, nullptr);
    }

    void PostProcessPass::RecreateResources()
    {
        UpdateInputSet(m_inputSet);
    }

    void PostProcessPass::Draw(const FrameInfo& frameInfo) const
    {
        assert(m_pipeline && "PostProcessPass: pipeline is null");

        auto& commandBuffer = frameInfo.m_commandBuffer;
        auto& imageIndex = frameInfo.m_imageIndex;

        VkImageSubresourceRange colorRange{};
        colorRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorRange.baseMipLevel = 0;
        colorRange.levelCount = 1;
        colorRange.baseArrayLayer = 0;
        colorRange.layerCount = 1;

        // Color image: COLOR_ATTACHMENT_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
        RenderImage* colorImage = m_resources.GetImage(m_target.m_colorTex.m_imageHandle);
        assert(colorImage && "PostProcessPass: Color image handle is invalid");
        TransitionImageLayout(commandBuffer,
            colorImage->m_image,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
            colorRange);

        // Swap chain image: UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
        TransitionImageLayout(commandBuffer,
            m_swapChain.Image(imageIndex),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            colorRange);

        VkExtent2D extent = m_swapChain.Extent();

        // Begin rendering
        {
            RenderingAttachmentDesc colorDesc{};
            colorDesc.m_imageView = m_swapChain.ImageView(imageIndex);
            colorDesc.m_imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorDesc.m_loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorDesc.m_storeOp = VK_ATTACHMENT_STORE_OP_STORE;

            RenderingScope scope(commandBuffer, extent, { colorDesc });

            // Bind pipeline
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Handle());

            // Draw
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                0, 1, &m_inputSet, 0, nullptr);
            vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        }
        // End rendering

        // Transition the image layout to present
        TransitionImageLayout(commandBuffer,
            m_swapChain.Image(imageIndex),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_ACCESS_2_NONE,
            colorRange);
    }

    void PostProcessPass::CreateDescriptorSetLayouts()
    {
        // Input layout
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings(1);
            bindings[0].binding = 0;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[0].descriptorCount = 1;
            bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            createInfo.pBindings = bindings.data();

            m_inputLayout = CreateDescriptorSetLayout(m_context.Device(), createInfo);
        }
    }

    void PostProcessPass::AllocateDescriptorSets()
    {
        // Input set
        {
            // Allocate
            m_inputSet = m_descriptorAllocator.Allocate(m_inputLayout, "PostProcess input set");

            // Setup
            UpdateInputSet(m_inputSet);
        }
    }

    void PostProcessPass::UpdateInputSet(VkDescriptorSet set) const
    {
        VkDescriptorImageInfo imageInfo{};
        {
            auto& texture = m_target.m_colorTex;

            RenderImageView* imageView = m_resources.GetImageView(texture.m_imageViewHandle);
            assert(imageView && "Image view handle is invalid");
            RenderSampler* sampler = m_resources.GetSampler(texture.m_samplerHandle);
            assert(sampler && "Sampler handle is invalid");

            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = imageView->m_imageView;
            imageInfo.sampler = sampler->m_sampler;
        }

        std::vector<VkWriteDescriptorSet> writes(1);
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = set;
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].descriptorCount = 1;
        writes[0].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_context.Device(),
            static_cast<uint32_t>(writes.size()), writes.data()
            , 0, nullptr);
    }

    void PostProcessPass::CreatePipeline()
    {
        GraphicsPipelineBuilder builder(m_context.Device());
        builder.SetShaders("assets/shaders/post_process_vert.spv", "assets/shaders/post_process_frag.spv")
            .SetDescriptorSetLayouts({ m_inputLayout })
            .SetDynamicRendering({ m_swapChain.Format() }, VK_FORMAT_UNDEFINED);
        m_pipeline = builder.Build();
    }
}
