#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>

namespace Kita::Pbrv
{
    struct ComputePipelineConfig
    {
        // Shader stage
        std::string m_compPath{};

        // Pipeline layout
        std::vector<VkDescriptorSetLayout> m_setLayouts{};
        std::vector<VkPushConstantRange> m_pushConstants{};
    };

    class ComputePipeline
    {
    public:
        ComputePipeline(VkDevice device, const ComputePipelineConfig& config);
        ~ComputePipeline();

        ComputePipeline(const ComputePipeline&) = delete;
        ComputePipeline& operator=(const ComputePipeline&) = delete;

        VkPipeline Handle() const { return m_pipeline; }
        VkPipelineLayout Layout() const { return m_layout; }

    private:
        VkDevice m_device{ VK_NULL_HANDLE };
        VkPipelineLayout m_layout{ VK_NULL_HANDLE };
        VkPipeline m_pipeline{ VK_NULL_HANDLE };
    };

    class ComputePipelineBuilder
    {
    public:
        explicit ComputePipelineBuilder(VkDevice device);

        ComputePipelineBuilder& SetShader(const std::string& compPath);
        ComputePipelineBuilder& SetDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout>& setLayouts);
        ComputePipelineBuilder& SetPushConstants(const std::vector<VkPushConstantRange>& pushConstants);

        std::unique_ptr<ComputePipeline> Build() const;

    private:
        VkDevice m_device{ VK_NULL_HANDLE };
        ComputePipelineConfig m_config{};
    };
}
