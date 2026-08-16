#include "graphics_pipeline.h"

#include "render/shader_module.h"

#include <stdexcept>

namespace Kita::Pbrv
{
    GraphicsPipeline::GraphicsPipeline(VkDevice device, const GraphicsPipelineConfig& config)
        : m_device(device)
    {
        // Shaders
        ShaderModule vertShaderModule(m_device, config.m_vertPath);
        ShaderModule fragShaderModule(m_device, config.m_fragPath);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule.Handle();
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule.Handle();
        fragShaderStageInfo.pName = "main";

        // Vertex input
        auto& vertexBindings = config.m_vertexBindings;
        auto& vertexAttributes = config.m_vertexAttributes;
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindings.size());
        vertexInputInfo.pVertexBindingDescriptions = vertexBindings.data();
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
        rasterizer.cullMode = config.m_cullMode;
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
        depthStencil.depthTestEnable = config.m_depthTestEnable;
        depthStencil.depthWriteEnable = config.m_depthWriteEnable;
        depthStencil.depthCompareOp = config.m_depthCompareOp;
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

        // Pipeline layout
        auto& setLayouts = config.m_setLayouts;
        auto& pushConstants = config.m_pushConstants;
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        pipelineLayoutInfo.pSetLayouts = setLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
        pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

        if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_layout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        // Dynamic rendering
        auto& colorAttachmentFormats = config.m_colorAttachmentFormats;
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentFormats.size());
        renderingInfo.pColorAttachmentFormats = colorAttachmentFormats.data();
        renderingInfo.depthAttachmentFormat = config.m_depthAttachmentFormat;

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
        pipelineInfo.layout = m_layout;
        pipelineInfo.renderPass = nullptr;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }
    }

    GraphicsPipeline::~GraphicsPipeline()
    {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_layout, nullptr);
    }

    GraphicsPipelineBuilder::GraphicsPipelineBuilder(VkDevice device)
        : m_device(device)
    {
    }

    GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetShaders(const std::string& vertPath, const std::string& fragPath)
    {
        m_config.m_vertPath = vertPath;
        m_config.m_fragPath = fragPath;
        return *this;
    }

    GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetVertexInput(const std::vector<VkVertexInputBindingDescription>& bindings, const std::vector<VkVertexInputAttributeDescription>& attributes)
    {
        m_config.m_vertexBindings = bindings;
        m_config.m_vertexAttributes = attributes;
        return *this;
    }

    GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetCullMode(VkCullModeFlags cullMode)
    {
        m_config.m_cullMode = cullMode;
        return *this;
    }

    GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetDepth(bool testEnable, bool writeEnable, VkCompareOp op)
    {
        m_config.m_depthTestEnable = testEnable;
        m_config.m_depthWriteEnable = writeEnable;
        m_config.m_depthCompareOp = op;
        return *this;
    }

    GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout>& setLayouts)
    {
        m_config.m_setLayouts = setLayouts;
        return *this;
    }

    GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetPushConstants(const std::vector<VkPushConstantRange>& pushConstants)
    {
        m_config.m_pushConstants = pushConstants;
        return *this;
    }

    GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetDynamicRendering(const std::vector<VkFormat>& colorFormats, VkFormat depthFormat)
    {
        m_config.m_colorAttachmentFormats = colorFormats;
        m_config.m_depthAttachmentFormat = depthFormat;
        return *this;
    }

    std::unique_ptr<GraphicsPipeline> GraphicsPipelineBuilder::Build() const
    {
        return std::make_unique<GraphicsPipeline>(m_device, m_config);
    }
}