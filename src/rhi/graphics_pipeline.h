#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        struct GraphicsPipelineConfig
        {
            // Shader stage
            std::string m_vertPath{};
            std::string m_fragPath{};

            // Vertex input
            std::vector<VkVertexInputBindingDescription> m_vertexBindings;
            std::vector<VkVertexInputAttributeDescription> m_vertexAttributes;

            // Rasterization
            VkCullModeFlags m_cullMode{ VK_CULL_MODE_NONE };

            // Multisample
            VkSampleCountFlagBits m_rasterizationSamples{ VK_SAMPLE_COUNT_1_BIT };

            // Depth and stencil
            VkBool32 m_depthTestEnable{ VK_FALSE };
            VkBool32 m_depthWriteEnable{ VK_FALSE };
            VkCompareOp m_depthCompareOp{ VK_COMPARE_OP_LESS };

            // Pipeline layout
            std::vector<VkDescriptorSetLayout> m_setLayouts{};
            std::vector<VkPushConstantRange> m_pushConstants{};

            // Dynamic rendering
            std::vector<VkFormat> m_colorAttachmentFormats{};
            VkFormat m_depthAttachmentFormat{ VK_FORMAT_UNDEFINED };
        };

        class GraphicsPipeline
        {
        public:
            GraphicsPipeline(VkDevice device, const GraphicsPipelineConfig& config);
            ~GraphicsPipeline();

            GraphicsPipeline(const GraphicsPipeline&) = delete;
            GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

            VkPipeline Handle() const { return m_pipeline; }
            VkPipelineLayout Layout() const { return m_layout; }

        private:
            VkDevice m_device{ VK_NULL_HANDLE };
            VkPipelineLayout m_layout{ VK_NULL_HANDLE };
            VkPipeline m_pipeline{ VK_NULL_HANDLE };
        };

        class GraphicsPipelineBuilder
        {
        public:
            explicit GraphicsPipelineBuilder(VkDevice device);

            GraphicsPipelineBuilder& SetShaders(const std::string& vertPath, const std::string& fragPath);
            GraphicsPipelineBuilder& SetVertexInput(const std::vector<VkVertexInputBindingDescription>& bindings,
                const std::vector<VkVertexInputAttributeDescription>& attributes);
            GraphicsPipelineBuilder& SetCullMode(VkCullModeFlags cullMode);
            GraphicsPipelineBuilder& SetRasterizationSamples(VkSampleCountFlagBits samples);
            GraphicsPipelineBuilder& SetDepth(bool testEnable, bool writeEnable, VkCompareOp op);
            GraphicsPipelineBuilder& SetDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout>& setLayouts);
            GraphicsPipelineBuilder& SetPushConstants(const std::vector<VkPushConstantRange>& pushConstants);
            GraphicsPipelineBuilder& SetDynamicRendering(const std::vector<VkFormat>& colorFormats, VkFormat depthFormat);

            std::unique_ptr<GraphicsPipeline> Build() const;

        private:
            VkDevice m_device{ VK_NULL_HANDLE };
            GraphicsPipelineConfig m_config{};
        };
    }
}
