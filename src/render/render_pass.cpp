#include "render_pass.h"

#include "render/render_context.h"
#include "render/render_utils.h"
#include "render/render_resources.h"
#include "render/swap_chain.h"

#include "scene/vertex.h"

#include <stdexcept>
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>

namespace Kita::Pbrv
{
    RenderPass::RenderPass(const RenderContext& context, RenderResources& resources, const SwapChain& swapChain)
        : m_context(context), m_resources(resources), m_swapChain(swapChain)
    {
        CreateDescriptorPool();
        CreateDescriptorSetLayouts();
        CreatePipeline();
        CreateDepthImage();
    }

    RenderPass::~RenderPass()
    {
        DestroyDepthImage();
        vkDestroyPipeline(m_context.Device(), m_pipeline, nullptr);
        vkDestroyPipelineLayout(m_context.Device(), m_pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(m_context.Device(), m_frameLayout, nullptr);
        vkDestroyDescriptorSetLayout(m_context.Device(), m_matLayout, nullptr);
        vkDestroyDescriptorPool(m_context.Device(), m_descriptorPool, nullptr);
    }

    void RenderPass::Initialize(const RenderList& list)
    {
        AllocateDescriptorSets(list);
    }

    void RenderPass::RecreateResources()
    {
        DestroyDepthImage();
        CreateDepthImage();
    }

    void RenderPass::Draw(const RenderList& list, const FrameInfo& frameInfo)
    {
        auto& commandBuffer = frameInfo.m_commandBuffer;
        auto& frameIndex = frameInfo.m_frameIndex;
        auto& imageIndex = frameInfo.m_imageIndex;

        // Transition the image layout to color attachment optimal
        TransitionImageLayout(commandBuffer,
            m_swapChain.Image(imageIndex),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

        // Transition depth image layout
        auto depthImage = m_resources.GetImage(m_depthImageHandle);
        assert(depthImage && "Depth image handle is invalid");
        TransitionImageLayout(commandBuffer,
            depthImage->m_image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT);

        // Begin rendering
        std::vector<VkClearValue> clearValues(2, VkClearValue{});
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        VkExtent2D extent = m_swapChain.Extent();

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = m_swapChain.ImageView(imageIndex);
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
        colorAttachment.resolveImageView = VK_NULL_HANDLE;
        colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = clearValues[0];

        RenderImageView* imageView = m_resources.GetImageView(m_depthImageViewHandle);
        assert(imageView && "Depth image view handle is invalid");
        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = imageView->m_imageView;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.clearValue = clearValues[1];

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        renderingInfo.renderArea.offset = { 0, 0 };
        renderingInfo.renderArea.extent = extent;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        renderingInfo.pDepthAttachment = &depthAttachment;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);

        // Viewport and scissor
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // Bind pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        // Draw
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
            0, 1, &m_frameSets[frameIndex], 0, nullptr);
        {
            UpdateMaterialDescriptorSet(frameIndex, list);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
                1, 1, &m_matSets[frameIndex], 0, nullptr);

            auto& pushConstant = list.m_material.m_pushConstant;
            vkCmdPushConstants(commandBuffer, m_pipelineLayout,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(pushConstant), &pushConstant);

            RenderBuffer* vertexBuffer = m_resources.GetBuffer(list.m_mesh.m_vertexBufferHandle);
            assert(vertexBuffer && "Vertex buffer handle is invalid");
            VkBuffer buffers[]{ vertexBuffer->m_buffer };
            VkDeviceSize offsets[]{ 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

            RenderBuffer* indexBuffer = m_resources.GetBuffer(list.m_mesh.m_indexBufferHandle);
            assert(indexBuffer && "Index buffer handle is invalid");
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer->m_buffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(commandBuffer, list.m_mesh.m_indexCount, 1, 0, 0, 0);
        }

        // End rendering
        vkCmdEndRendering(commandBuffer);

        // Transition the image layout to present
        TransitionImageLayout(commandBuffer,
            m_swapChain.Image(imageIndex),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_ACCESS_2_NONE);
    }

    void RenderPass::CreateDescriptorPool()
    {
        std::vector<VkDescriptorPoolSize> poolSizes(2);
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = kMaxFramesInFlight * 1;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = kMaxFramesInFlight * kMaterialTextureCount;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = kMaxFramesInFlight * 2;

        if (vkCreateDescriptorPool(m_context.Device(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create descriptor pool!");
        }
    }

    void RenderPass::CreateDescriptorSetLayouts()
    {
        // Frame layout
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings(1);
            bindings[0].binding = 0;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            bindings[0].descriptorCount = 1;
            bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            createInfo.pBindings = bindings.data();

            m_frameLayout = CreateDescriptorSetLayout(m_context.Device(), createInfo);
        }

        // Material layout
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings(1);
            bindings[0].binding = 0;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[0].descriptorCount = 5;
            bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            createInfo.pBindings = bindings.data();

            m_matLayout = CreateDescriptorSetLayout(m_context.Device(), createInfo);
        }
    }

    void RenderPass::AllocateDescriptorSets(const RenderList& list)
    {
        // Frame set
        {
            // Allocate
            m_frameSets.resize(kMaxFramesInFlight);
            std::vector<VkDescriptorSetLayout> layouts(kMaxFramesInFlight, m_frameLayout);

            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = m_descriptorPool;
            allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
            allocInfo.pSetLayouts = layouts.data();

            if (vkAllocateDescriptorSets(m_context.Device(), &allocInfo, m_frameSets.data()) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to allocate descriptor sets!");
            }

            // Setup
            for (size_t i = 0; i < m_frameSets.size(); ++i)
            {
                auto& set = m_frameSets[i];

                VkDescriptorBufferInfo bufferInfo{};
                {
                    RenderBuffer* buffer = m_resources.GetBuffer(list.m_frame.m_uboHandles[i]);
                    assert(buffer && "Frame buffer handle is invalid");
                    bufferInfo.buffer = buffer->m_buffer;
                    bufferInfo.offset = 0;
                    bufferInfo.range = sizeof(FrameUbo);
                }

                std::vector<VkWriteDescriptorSet> writes(1);
                writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writes[0].dstSet = set;
                writes[0].dstBinding = 0;
                writes[0].dstArrayElement = 0;
                writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                writes[0].descriptorCount = 1;
                writes[0].pBufferInfo = &bufferInfo;

                vkUpdateDescriptorSets(m_context.Device(),
                    static_cast<uint32_t>(writes.size()), writes.data()
                    , 0, nullptr);
            }
        }

        // Material set
        {
            // Allocate
            m_matSets.resize(kMaxFramesInFlight);
            std::vector<VkDescriptorSetLayout> layouts(kMaxFramesInFlight, m_matLayout);

            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = m_descriptorPool;
            allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
            allocInfo.pSetLayouts = layouts.data();

            if (vkAllocateDescriptorSets(m_context.Device(), &allocInfo, m_matSets.data()) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to allocate descriptor sets!");
            }

            // Setup when drawing(if necessary)
        }
    }

    void RenderPass::UpdateMaterialDescriptorSet(uint32_t frameIndex, const RenderList& list)
    {
        auto& bound = m_boundTextures[frameIndex];
        if (bound == list.m_material.m_textures)
        {
            return;
        }

        auto& matSet = m_matSets[frameIndex];
        std::array<VkDescriptorImageInfo, kMaterialTextureCount> imageInfos{};
        for (size_t i = 0; i < imageInfos.size(); ++i)
        {
            auto slot = MaterialTextureSlot(i);
            auto& texture = list.m_material.m_textures[i];

            std::clog << "[RenderPass] Update texture " << ToString(slot) << ", "
                << "image view (" << texture.m_imageViewHandle << "), "
                << "sampler (" << texture.m_samplerHandle << ")\n";

            RenderImageView* imageView = m_resources.GetImageView(texture.m_imageViewHandle);
            assert(imageView && "Image view handle is invalid");
            RenderSampler* sampler = m_resources.GetSampler(texture.m_samplerHandle);
            assert(sampler && "Sampler handle is invalid");

            auto& imageInfo = imageInfos[i];
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = imageView->m_imageView;
            imageInfo.sampler = sampler->m_sampler;
        }

        std::array<VkWriteDescriptorSet, 1> writes{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = matSet;
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].descriptorCount = static_cast<uint32_t>(imageInfos.size());
        writes[0].pImageInfo = imageInfos.data();

        vkUpdateDescriptorSets(m_context.Device(),
            static_cast<uint32_t>(writes.size()), writes.data()
            , 0, nullptr);

        bound = list.m_material.m_textures;
    }

    void RenderPass::CreatePipeline()
    {
        // Shaders
        VkShaderModule vertShaderModule = CreateShaderModule("assets/shaders/lit_vert.spv");
        VkShaderModule fragShaderModule = CreateShaderModule("assets/shaders/lit_frag.spv");

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        // Vertex input
        auto vertexBinding = Vertex::GetBindingDescription();
        auto vertexAttributes = Vertex::GetAttributeDescriptions();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &vertexBinding;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size());
        vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.data();

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // Rasterization
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        // Depth and stencil
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};

        // Color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        // dynamic state
        std::vector<VkDynamicState> dynamicStates
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // Push constant
        VkPushConstantRange pushConstant{};
        pushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstant.offset = 0;
        pushConstant.size = sizeof(MaterialPC);

        // Pipeline layout
        std::vector<VkDescriptorSetLayout> setLayouts
        {
            m_frameLayout,
            m_matLayout
        };
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        pipelineLayoutInfo.pSetLayouts = setLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

        if (vkCreatePipelineLayout(m_context.Device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        // Dynamic rendering
        std::vector<VkFormat> colorAttachmentFormats
        {
            m_swapChain.Format()
        };
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentFormats.size());
        renderingInfo.pColorAttachmentFormats = colorAttachmentFormats.data();
        renderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

        // Pipeline
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages
        {
            vertShaderStageInfo,
            fragShaderStageInfo
        };
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = &renderingInfo;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_pipelineLayout;
        pipelineInfo.renderPass = nullptr;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(m_context.Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(m_context.Device(), vertShaderModule, nullptr);
        vkDestroyShaderModule(m_context.Device(), fragShaderModule, nullptr);
    }

    void RenderPass::CreateDepthImage()
    {
        VkExtent2D extent = m_swapChain.Extent();

        // Render image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = extent.width;
        imageInfo.extent.height = extent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_D32_SFLOAT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        m_depthImageHandle = m_resources.CreateImage(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        // Image view
        auto depthImage = m_resources.GetImage(m_depthImageHandle);
        assert(depthImage && "Depth image handle is invalid");
        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.image = depthImage->m_image;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = imageInfo.format;
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;

        m_depthImageViewHandle = m_resources.CreateImageView(imageViewInfo);
    }

    void RenderPass::DestroyDepthImage()
    {
        m_resources.DestroyImageView(m_depthImageViewHandle);
        m_resources.DestroyImage(m_depthImageHandle);
    }

    VkShaderModule RenderPass::CreateShaderModule(const std::string& filePath) const
    {
        auto code = ReadFile(filePath);

        VkShaderModule shaderModule;

        VkShaderModuleCreateInfo shaderInfo{};
        shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderInfo.codeSize = code.size();
        shaderInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        if (vkCreateShaderModule(m_context.Device(), &shaderInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create shader module!");
        }

        return shaderModule;
    }

    std::vector<char> RenderPass::ReadFile(const std::string& path) const
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file " + path + "!");
        }

        size_t filesize = (size_t)file.tellg();
        std::vector<char> buffer(filesize);

        file.seekg(0);
        file.read(buffer.data(), filesize);

        file.close();

        return buffer;
    }
}