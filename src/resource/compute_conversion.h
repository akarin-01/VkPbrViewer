#pragma once

#include "resource/descriptor_manager.h"
#include "resource/types.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include <vector>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class RenderContext;
        class ComputePipeline;
    }

    namespace Resource
    {
        class RenderResources;
        class DescriptorManager;

        class ComputeConversion
        {
        public:
            struct Input
            {
                VkDescriptorType m_type{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
                VkImageLayout m_layout{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                RenderImageViewHandle m_imageView{ 0 };
                RenderSamplerHandle m_sampler{ 0 };
            };

            struct Output
            {
                RenderImageHandle m_image{ 0 };
                RenderImageViewHandle m_imageView{ 0 };
                VkImageSubresourceRange m_range{};
                VkImageLayout m_finalLayout{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                VkPipelineStageFlags2 m_finalStage{ VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT };
                VkAccessFlags2 m_finalAccess{ VK_ACCESS_2_SHADER_READ_BIT };
            };

            ComputeConversion(const Rhi::RenderContext& context,
                RenderResources& resources,
                DescriptorManager& descriptorMgr,
                DescriptorLayoutType layoutType,
                const std::string& shaderPath, uint32_t pushConstantSize);
            ~ComputeConversion();

            ComputeConversion(const ComputeConversion&) = delete;
            ComputeConversion& operator=(const ComputeConversion&) = delete;
            ComputeConversion(ComputeConversion&&) = delete;
            ComputeConversion& operator=(ComputeConversion&&) = delete;

            // Runs the kernel synchronously: the output is transitioned UNDEFINED->GENERAL,
            // then to finalLayout, and inputs are bound to bindings 1..n in order.
            void Dispatch(const Output& output, VkExtent3D dispatchSize,
                const std::vector<Input>& inputs,
                const void* pushData = nullptr) const;

        private:
            const Rhi::RenderContext& m_context;
            RenderResources& m_resources;
            DescriptorManager& m_descriptorMgr;

            // Count as the default: an unset layout type fails the manager's assert
            // instead of silently binding to a real layout
            DescriptorLayoutType m_layoutType{ DescriptorLayoutType::Count };

            uint32_t m_pushConstantSize{ 0 };

            VkDescriptorSet m_set{ VK_NULL_HANDLE };
            std::unique_ptr<Rhi::ComputePipeline> m_pipeline;
        };
    }
}
