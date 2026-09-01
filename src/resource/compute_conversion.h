#pragma once

#include "resource/resource_types.h"

#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class ComputePipeline;
        class Context;
    }

    namespace Resource
    {
        class DescriptorManager;

        /// One-shot compute dispatch helper: owns a compute pipeline + set,
        /// writes bindings (output storage image at 0, inputs at 1..n) and
        /// transitions the output UNDEFINED -> GENERAL -> finalLayout.
        class ComputeConversion
        {
        public:
            struct Input
            {
                VkDescriptorType m_type{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
                VkImageLayout m_layout{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                VkImageView m_imageView{ VK_NULL_HANDLE };
                VkSampler m_sampler{ VK_NULL_HANDLE };
            };

            struct Output
            {
                VkImage m_image{ VK_NULL_HANDLE };
                VkImageView m_imageView{ VK_NULL_HANDLE };
                VkImageSubresourceRange m_range{};
                VkImageLayout m_finalLayout{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                VkPipelineStageFlags2 m_finalStage{ VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT };
                VkAccessFlags2 m_finalAccess{ VK_ACCESS_2_SHADER_READ_BIT };
            };

            ComputeConversion(const Rhi::Context& context,
                DescriptorManager& descriptorMgr,
                DescriptorSetRhi::Type layoutType,
                const std::string& shaderPath, uint32_t pushConstantSize);
            ~ComputeConversion();

            ComputeConversion(const ComputeConversion&) = delete;
            ComputeConversion& operator=(const ComputeConversion&) = delete;
            ComputeConversion(ComputeConversion&&) = delete;
            ComputeConversion& operator=(ComputeConversion&&) = delete;

            // Synchronous dispatch: output UNDEFINED -> GENERAL -> finalLayout;
            // inputs are bound to bindings 1..n in order.
            void Dispatch(const Output& output, VkExtent3D dispatchSize,
                const std::vector<Input>& inputs,
                const void* pushData = nullptr) const;

        private:
            const Rhi::Context& m_context;
            DescriptorManager& m_descriptorMgr;

            // Count: an unset type hits the manager's assert, never a silent wrong layout
            DescriptorSetRhi::Type m_layoutType{ DescriptorSetRhi::Type::Count };

            uint32_t m_pushConstantSize{ 0 };

            VkDescriptorSet m_set{ VK_NULL_HANDLE };
            std::unique_ptr<Rhi::ComputePipeline> m_pipeline;
        };
    }
}
