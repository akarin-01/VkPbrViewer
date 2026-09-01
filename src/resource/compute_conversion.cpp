#include "compute_conversion.h"

#include "rhi/compute_pipeline.h"
#include "rhi/context.h"
#include "rhi/one_shot_command.h"
#include "rhi/utils.h"
#include "resource/descriptor_manager.h"
#include "resource/descriptor_writer.h"

#include <cassert>

namespace Kita::Pbrv
{
    namespace Resource
    {
        ComputeConversion::ComputeConversion(const Rhi::Context& context,
            DescriptorManager& descriptorMgr,
            DescriptorSetRhi::Type layoutType,
            const std::string& shaderPath, uint32_t pushConstantSize)
            : m_context(context),
            m_descriptorMgr(descriptorMgr),
            m_layoutType(layoutType),
            m_pushConstantSize(pushConstantSize)
        {
            // Set
            {
                m_set = m_descriptorMgr.Allocate(m_layoutType);
            }

            // Pipeline
            {
                Rhi::ComputePipelineBuilder builder(m_context.Device());
                builder.SetShader(shaderPath)
                    .SetDescriptorSetLayouts({ m_descriptorMgr.GetLayout(m_layoutType) });

                if (pushConstantSize > 0)
                {
                    VkPushConstantRange pushConstant{};
                    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                    pushConstant.offset = 0;
                    pushConstant.size = pushConstantSize;

                    builder.SetPushConstants({ pushConstant });
                }

                m_pipeline = builder.Build();
            }
        }

        ComputeConversion::~ComputeConversion()
        {
            m_pipeline.reset();

            // Sets will be destroyed automatically
        }

        void ComputeConversion::Dispatch(const Output& output, VkExtent3D dispatchSize,
            const std::vector<Input>& inputs,
            const void* pushData) const
        {
            assert(m_pipeline && "ComputeConversion: pipeline is null");

            // 1. Write set
            {
                DescriptorWriter writer(m_context.Device());
                writer.WriteImage(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    VK_IMAGE_LAYOUT_GENERAL, output.m_imageView, VK_NULL_HANDLE);
                for (size_t i = 0; i < inputs.size(); ++i)
                {
                    auto& input = inputs[i];
                    if (input.m_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    {
                        writer.WriteImage(static_cast<uint32_t>(i + 1), input.m_type,
                            input.m_layout, input.m_imageView, input.m_sampler);
                    }
                }
                writer.UpdateSet(m_set);
            }

            // 2. Dispatch conversion
            {
                Rhi::OneShotCommand command(m_context);

                // Output: UNDEFINED -> GENERAL
                Rhi::TransitionImageLayout(command.Handle(), output.m_image,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                    VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                    VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                    output.m_range);

                if (m_pushConstantSize > 0)
                {
                    assert(pushData && "ComputeConversion: push data is null");
                    vkCmdPushConstants(command.Handle(), m_pipeline->Layout(), VK_SHADER_STAGE_COMPUTE_BIT,
                        0, m_pushConstantSize, pushData);
                }
                vkCmdBindPipeline(command.Handle(), VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->Handle());
                vkCmdBindDescriptorSets(command.Handle(), VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->Layout(),
                    0, 1, &m_set, 0, nullptr);
                vkCmdDispatch(command.Handle(), dispatchSize.width, dispatchSize.height, dispatchSize.depth);

                // Output: GENERAL -> final layout
                Rhi::TransitionImageLayout(command.Handle(), output.m_image,
                    VK_IMAGE_LAYOUT_GENERAL, output.m_finalLayout,
                    VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                    output.m_finalStage, output.m_finalAccess,
                    output.m_range);
            }
        }
    }
}
