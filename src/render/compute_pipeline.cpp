#include "compute_pipeline.h"

#include "render/shader_module.h"

#include <stdexcept>

namespace Kita::Pbrv
{
    ComputePipeline::ComputePipeline(VkDevice device, const ComputePipelineConfig& config)
        : m_device(device)
    {
        ShaderModule compShaderModule(m_device, config.m_compPath);

        // Shader stage
        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stageInfo.module = compShaderModule.Handle();
        stageInfo.pName = "main";

        // Pipeline layout
        auto& setLayouts = config.m_setLayouts;
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        pipelineLayoutInfo.pSetLayouts = setLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_layout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        // Pipeline
        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage = stageInfo;
        pipelineInfo.layout = m_layout;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create compute pipeline!");
        }
    }

    ComputePipeline::~ComputePipeline()
    {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_layout, nullptr);
    }

    ComputePipelineBuilder::ComputePipelineBuilder(VkDevice device)
        : m_device(device)
    {
    }

    ComputePipelineBuilder& ComputePipelineBuilder::SetShader(const std::string& compPath)
    {
        m_config.m_compPath = compPath;
        return *this;
    }

    ComputePipelineBuilder& ComputePipelineBuilder::SetDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout>& setLayouts)
    {
        m_config.m_setLayouts = setLayouts;
        return *this;
    }

    std::unique_ptr<ComputePipeline> ComputePipelineBuilder::Build() const
    {
        return std::make_unique<ComputePipeline>(m_device, m_config);
    }
}